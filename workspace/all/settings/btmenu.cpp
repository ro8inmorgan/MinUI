#include "btmenu.hpp"
#include "keyboardprompt.hpp"
#ifdef HAS_BTAGENT
#   include "btagent.hpp"
#endif

#include <unordered_set>
#include <map>

#include <mutex>
#include <shared_mutex>
typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

using namespace Bluetooth;
using namespace std::placeholders;

Menu::Menu(const int &, int &) : MenuList(MenuItemType::Fixed, "Network", {})
{
    toggleItem = new MenuItem(ListItemType::Generic, "Bluetooth", "Enable/disable Bluetooth", {false, true}, {"Off", "On"},
                              std::bind(&Menu::getBtToggleState, this),
                              std::bind(&Menu::setBtToggleState, this, std::placeholders::_1),
                              std::bind(&Menu::resetBtToggleState, this));
    diagItem = new MenuItem(ListItemType::Generic, "Bluetooth diagnostics", "Enable/disable Bluetooth logging", {false, true}, {"Off", "On"},
                              std::bind(&Menu::getBtDiagnosticsState, this),
                              std::bind(&Menu::setBtDiagnosticsState, this, std::placeholders::_1),
                              std::bind(&Menu::resetBtDiagnosticsState, this));
    rateItem = new MenuItem(ListItemType::Generic, "Maximum sampling rate", "44100 Hz: better compatibility\n48000 Hz: better quality", {44100, 48000}, {"44100 Hz", "48000 Hz"},
                              std::bind(&Menu::getSamplerateMaximum, this),
                              std::bind(&Menu::setSamplerateMaximum, this, std::placeholders::_1),
                              std::bind(&Menu::resetSamplerateMaximum, this));
    items.push_back(toggleItem);
    items.push_back(diagItem);
    items.push_back(rateItem);

    // best effort layout based on the platform defines, user should really call performLayout manually
    MenuList::performLayout((SDL_Rect){0, 0, FIXED_WIDTH, FIXED_HEIGHT});
    layout_called = false;

#ifdef HAS_BTAGENT
    // Only NoInputNoOutput for now, but this needs to interact with the UI thread if we 
    // ever want to show a PIN or passkey
    pairingAgent = new PairingAgent();
    pairingAgent->startPairingWindow();
#endif

    worker = std::thread{&Menu::updater, this};
}

Menu::~Menu()
{
    quit = true;
    if (worker.joinable())
        worker.join();

#ifdef HAS_BTAGENT
    pairingAgent->stopPairingWindow();
    delete pairingAgent;
#endif
}

InputReactionHint Menu::handleInput(int &dirty, int &quit)
{
    applySnapshot(dirty);
    auto ret = MenuList::handleInput(dirty, quit);
    if (selectionDirty) {
        selectionDirty = false;
        dirty = true;
    }
    return ret;
}

std::any Menu::getBtToggleState() const
{
    return BT_enabled();
}

void Menu::setBtToggleState(const std::any &on)
{
    auto state = std::any_cast<bool>(on);
    ScopedOverlay overlay(state ? "Enabling Bluetooth..." : "Disabling Bluetooth...");
    BT_enable(state);
}

void Menu::resetBtToggleState()
{
    //
}

std::any Menu::getBtDiagnosticsState() const
{
    return BT_diagnosticsEnabled();
}

void Menu::setBtDiagnosticsState(const std::any &on)
{
    BT_diagnosticsEnable(std::any_cast<bool>(on));
}

void Menu::resetBtDiagnosticsState()
{
    //
}

std::any Menu::getSamplerateMaximum() const
{
    return CFG_getBluetoothSamplingrateLimit();
}

void Menu::setSamplerateMaximum(const std::any &value)
{
    CFG_setBluetoothSamplingrateLimit(std::any_cast<int>(value));
}

void Menu::resetSamplerateMaximum()
{
    CFG_setBluetoothSamplingrateLimit(CFG_DEFAULT_BLUETOOTH_MAXRATE);
}

void Menu::applySnapshot(int &dirty)
{
    for (auto item : items)
        if (item->isDeferred()) return;

    ScanSnapshot latest;
    {
        std::lock_guard<std::mutex> lock(snapshotMutex);
        if (!snapshotReady) return;
        latest = std::move(snapshot);
        snapshotReady = false;
    }

    const std::string selectedName = getSelectedItemName();
    clearDynamicItems(3);
    if (latest.enabled) {
        for (const auto &device : latest.available)
            items.push_back(new PairableItem{device, new MenuList(MenuItemType::List, "Options", {new PairNewItem(device, selectionDirty)})});
        for (const auto &device : latest.paired) {
            MenuList *options = device.is_connected
                ? new MenuList(MenuItemType::List, "Options", {new DisconnectKnownItem(device, selectionDirty), new UnpairItem(device, selectionDirty)})
                : new MenuList(MenuItemType::List, "Options", {new ConnectKnownItem(device, selectionDirty), new UnpairItem(device, selectionDirty)});
            auto item = new PairedItem{device, options};
            item->setDesc(std::string(device.remote_addr) + " | " + std::to_string(device.rssi));
            items.push_back(item);
        }
    }
    layout_called = false;
    MenuList::performLayout((SDL_Rect){0, 0, FIXED_WIDTH, FIXED_HEIGHT});
    selectByName(selectedName);
    dirty = true;
}

void Menu::updater()
{
    ScanSnapshot previous;
    bool havePrevious = false;
    while (!quit) {
        ScanSnapshot latest;
        bool success = true;
        latest.enabled = BT_enabled();
        if (latest.enabled) {
            if (!BT_discovering()) BT_discovery(true);
            std::vector<BT_devicePaired> paired(SCAN_MAX_RESULTS);
            int pairedCount = BT_pairedDevices(paired.data(), SCAN_MAX_RESULTS);
            std::vector<BT_device> available(SCAN_MAX_RESULTS);
            int availableCount = pairedCount < 0 ? -1 : BT_availableDevices(available.data(), SCAN_MAX_RESULTS);
            if (pairedCount < 0 || availableCount < 0) success = false;
            else {
                std::map<std::string, BT_devicePaired> uniquePaired;
                std::map<std::string, BT_device> uniqueAvailable;
                for (int i = 0; i < pairedCount; i++) uniquePaired.emplace(paired[i].remote_addr, paired[i]);
                for (int i = 0; i < availableCount; i++) uniqueAvailable.emplace(available[i].name, available[i]);
                for (const auto &entry : uniqueAvailable) latest.available.push_back(entry.second);
                for (const auto &entry : uniquePaired) latest.paired.push_back(entry.second);
            }
        }
        if (success && (!havePrevious || latest.enabled != previous.enabled ||
            latest.available.size() != previous.available.size() || latest.paired.size() != previous.paired.size() ||
            !std::equal(latest.available.begin(), latest.available.end(), previous.available.begin(), [](const BT_device &a, const BT_device &b) {
                return strcmp(a.addr, b.addr) == 0 && strcmp(a.name, b.name) == 0 && a.kind == b.kind;
            }) || !std::equal(latest.paired.begin(), latest.paired.end(), previous.paired.begin(), [](const BT_devicePaired &a, const BT_devicePaired &b) {
                return strcmp(a.remote_addr, b.remote_addr) == 0 && strcmp(a.remote_name, b.remote_name) == 0 && a.rssi == b.rssi && a.is_bonded == b.is_bonded && a.is_connected == b.is_connected;
            }))) {
            std::lock_guard<std::mutex> lock(snapshotMutex);
            snapshot = latest;
            snapshotReady = true;
            previous = std::move(latest);
            havePrevious = true;
        }
        for (int i = 0; i < (success ? 20 : 150) && !quit; i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

PairNewItem::PairNewItem(BT_device d, bool& dirty)
    : MenuItem(ListItemType::Button, "Pair", "Pair this device.", 
        [&](AbstractMenuItem &item) -> InputReactionHint {
            ScopedOverlay overlay("Pairing...");
            BT_pair(dev.addr); 
            dirty = true;
            return Exit; 
        }), dev(d)
{}

UnpairItem::UnpairItem(BT_devicePaired d, bool& dirty)
    : MenuItem(ListItemType::Button, "Forget", "Forget this device.",
        [&](AbstractMenuItem &item) -> InputReactionHint {
            BT_unpair(dev.remote_addr); 
            dirty = true;
            return Exit; 
        }), dev(d)
{}

ConnectKnownItem::ConnectKnownItem(BT_devicePaired d, bool& dirty)
    : MenuItem(ListItemType::Button, "Connect", "Connect this device.",
        [&](AbstractMenuItem &item) -> InputReactionHint {
            ScopedOverlay overlay("Connecting...");
            BT_connect(dev.remote_addr); 
            dirty = true;
            return Exit; 
        }), dev(d)
{}

DisconnectKnownItem::DisconnectKnownItem(BT_devicePaired d, bool& dirty)
    : MenuItem(ListItemType::Button, "Disconnect", "Disconnect this device.",
        [&](AbstractMenuItem &item) -> InputReactionHint {
            BT_disconnect(dev.remote_addr); 
            dirty = true;
            return Exit; 
        }), dev(d)
{}

PairableItem::PairableItem(BT_device d, MenuList* submenu)
    : MenuItem(ListItemType::Custom, d.name, d.addr, DeferToSubmenu, submenu), dev(d)
{}

void PairableItem::drawCustomItem(SDL_Surface *surface, const SDL_Rect &dst, const AbstractMenuItem &item, bool selected) const
{
    SDL_Color text_color = uintToColour(THEME_COLOR4_255);
    SDL_Surface *text = TTF_RenderUTF8_Blended(font.tiny, item.getLabel().c_str(), COLOR_WHITE); // always white

    // hack - this should be correlated to max_width
    int mw = dst.w;

    if (selected)
    {
        // gray pill
        GFX_blitPillLightCPP(ASSET_BUTTON, surface, {dst.x, dst.y, mw, SCALE1(BUTTON_SIZE)});
    }

    // device icon
    if(dev.kind != BLUETOOTH_NONE) {
        auto asset = (dev.kind == BLUETOOTH_AUDIO) ? ASSET_AUDIO : ASSET_CONTROLLER;
        SDL_Rect rect = (dev.kind == BLUETOOTH_AUDIO) ? SDL_Rect{0, 0, 12, 12} : SDL_Rect{0, 0, 12, 12};
        int ix = dst.x + dst.w - SCALE1(OPTION_PADDING + rect.w);
        int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
        SDL_Rect tgt{ix, y};
        GFX_blitAssetColor(asset, NULL, surface, &tgt, THEME_COLOR6);
    }

    if (selected)
    {
        // white pill
        int w = 0;
        TTF_SizeUTF8(font.small, item.getName().c_str(), &w, NULL);
        w += SCALE1(OPTION_PADDING * 2);
        GFX_blitPillDarkCPP(ASSET_BUTTON, surface, {dst.x, dst.y, w, SCALE1(BUTTON_SIZE)});
        text_color = uintToColour(THEME_COLOR5_255);
    }

    text = TTF_RenderUTF8_Blended(font.small, item.getName().c_str(), text_color);
    SDL_BlitSurfaceCPP(text, {}, surface, {dst.x + SCALE1(OPTION_PADDING), dst.y + SCALE1(1)});
    SDL_FreeSurface(text);
}

PairedItem::PairedItem(BT_devicePaired d, MenuList* submenu)
    : MenuItem(ListItemType::Custom, d.remote_name, d.remote_addr, DeferToSubmenu, submenu), dev(d)
{}

void PairedItem::drawCustomItem(SDL_Surface *surface, const SDL_Rect &dst, const AbstractMenuItem &item, bool selected) const
{
    SDL_Color text_color = uintToColour(THEME_COLOR4_255);
    SDL_Surface *text = TTF_RenderUTF8_Blended(font.tiny, item.getLabel().c_str(), COLOR_WHITE); // always white

    // hack - this should be correlated to max_width
    int mw = dst.w;

    if (selected)
    {
        // gray pill
        GFX_blitPillLightCPP(ASSET_BUTTON, surface, {dst.x, dst.y, mw, SCALE1(BUTTON_SIZE)});
    }

    // rssi icon
    auto asset =
        dev.rssi == 0   ? ASSET_WIFI_OFF : 
        dev.rssi >= -55 ? ASSET_WIFI :
        dev.rssi >= -67 ? ASSET_WIFI_MED
                        : ASSET_WIFI_LOW;
    SDL_Rect rect = {0, 0, 12, 12};
    int ix = dst.x + dst.w - SCALE1(OPTION_PADDING + rect.w);
    int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
    SDL_Rect tgt{ix, y};
    GFX_blitAssetColor(asset, NULL, surface, &tgt, THEME_COLOR6);

    // connected
    if(dev.is_connected) {
        SDL_Rect rect = {0, 0, 12, 12};
        ix = ix - SCALE1(OPTION_PADDING + rect.w);
        int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
        SDL_Rect tgt{ix, y};
        GFX_blitAssetColor(ASSET_CHECKCIRCLE, NULL, surface, &tgt, THEME_COLOR6);
    }
    // bonded
    else if(dev.is_bonded) {
        SDL_Rect rect = {0, 0, 8, 11};
        ix = ix - SCALE1(OPTION_PADDING + rect.w + 2);
        int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
        SDL_Rect tgt{ix, y};
        GFX_blitAssetColor(ASSET_LOCK, NULL, surface, &tgt, THEME_COLOR6);
    }

    if (selected)
    {
        // white pill
        int w = 0;
        TTF_SizeUTF8(font.small, item.getName().c_str(), &w, NULL);
        w += SCALE1(OPTION_PADDING * 2);
        GFX_blitPillDarkCPP(ASSET_BUTTON, surface, {dst.x, dst.y, w, SCALE1(BUTTON_SIZE)});
        text_color = uintToColour(THEME_COLOR5_255);
    }

    text = TTF_RenderUTF8_Blended(font.small, item.getName().c_str(), text_color);
    SDL_BlitSurfaceCPP(text, {}, surface, {dst.x + SCALE1(OPTION_PADDING), dst.y + SCALE1(1)});
    SDL_FreeSurface(text);
}
