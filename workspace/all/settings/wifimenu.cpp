#include "wifimenu.hpp"
#include "keyboardprompt.hpp"

#include <unordered_set>
#include <map>

#include <mutex>
#include <shared_mutex>
typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

using namespace Wifi;
using namespace std::placeholders;

Menu::Menu(const int &, int &) : MenuList(MenuItemType::Fixed, "Network", {})
{
    toggleItem = new MenuItem(ListItemType::Generic, "WiFi", "Enable/disable WiFi", {false, true}, {"Off", "On"},
                              std::bind(&Menu::getWifToggleState, this),
                              std::bind(&Menu::setWifiToggleState, this, std::placeholders::_1),
                              std::bind(&Menu::resetWifiToggleState, this));
    diagItem = new MenuItem(ListItemType::Generic, "WiFi diagnostics", "Enable/disable WiFi logging", {false, true}, {"Off", "On"},
                              std::bind(&Menu::getWifDiagnosticsState, this),
                              std::bind(&Menu::setWifiDiagnosticsState, this, std::placeholders::_1),
                              std::bind(&Menu::resetWifiDiagnosticsState, this));
    items.push_back(toggleItem);
    items.push_back(diagItem);

    // best effort layout based on the platform defines, user should really call performLayout manually
    MenuList::performLayout((SDL_Rect){0, 0, FIXED_WIDTH, FIXED_HEIGHT});
    layout_called = false;

    worker = std::thread{&Menu::updater, this};
}

Menu::~Menu()
{
    quit = true;
    if (worker.joinable())
        worker.join();
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

std::any Menu::getWifToggleState() const
{
    return WIFI_enabled();
}

void Menu::setWifiToggleState(const std::any &on)
{
    auto state = std::any_cast<bool>(on);
    ScopedOverlay overlay(state ? "Enabling WiFi..." : "Disabling WiFi...");
    WIFI_enable(state);
}

void Menu::resetWifiToggleState()
{
    //
}

std::any Menu::getWifDiagnosticsState() const
{
    return WIFI_diagnosticsEnabled();
}

void Menu::setWifiDiagnosticsState(const std::any &on)
{
    WIFI_diagnosticsEnable(std::any_cast<bool>(on));
}

void Menu::resetWifiDiagnosticsState()
{
    //
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
    clearDynamicItems(2);
    if (latest.enabled) {
        for (const auto &network : latest.networks) {
            const bool connected = strcmp(latest.connection.ssid, network.ssid) == 0;
            MenuList *options = connected
                ? new MenuList(MenuItemType::List, "Options", {new MenuItem{ListItemType::Button, "Disconnect", "Disconnect from this network.", [&](AbstractMenuItem &) { WIFI_disconnect(); selectionDirty = true; return Exit; }}, new ForgetItem(network, selectionDirty)})
                : WIFI_isKnown(network.ssid, network.security)
                    ? new MenuList(MenuItemType::List, "Options", {new ConnectKnownItem(network, selectionDirty), new ForgetItem(network, selectionDirty)})
                    : new MenuList(MenuItemType::List, "Options", {new ConnectNewItem(network, selectionDirty)});
            auto item = new NetworkItem{network, connected, options};
            if (connected && latest.connection.ip[0]) item->setDesc(std::string(network.bssid) + " | " + latest.connection.ip);
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
        latest.enabled = WIFI_enabled();
        if (latest.enabled) {
            if (WIFI_connectionInfo(&latest.connection) < 0) success = false;
            std::vector<WIFI_network> results(SCAN_MAX_RESULTS);
            int count = success ? WIFI_scan(results.data(), SCAN_MAX_RESULTS) : -1;
            if (count < 0) success = false;
            else {
                std::map<std::string, WIFI_network> unique;
                for (int i = 0; i < count; i++) unique.emplace(results[i].ssid, results[i]);
                for (const auto &entry : unique) latest.networks.push_back(entry.second);
            }
        }
        if (success && (!havePrevious || latest.enabled != previous.enabled ||
            latest.connection.valid != previous.connection.valid || strcmp(latest.connection.ssid, previous.connection.ssid) != 0 ||
            strcmp(latest.connection.ip, previous.connection.ip) != 0 || latest.connection.freq != previous.connection.freq ||
            latest.connection.rssi != previous.connection.rssi || latest.connection.link_speed != previous.connection.link_speed ||
            latest.connection.noise != previous.connection.noise ||
            latest.networks.size() != previous.networks.size() ||
            !std::equal(latest.networks.begin(), latest.networks.end(), previous.networks.begin(), [](const WIFI_network &a, const WIFI_network &b) {
                return strcmp(a.bssid, b.bssid) == 0 && strcmp(a.ssid, b.ssid) == 0 && a.freq == b.freq && a.rssi == b.rssi && a.security == b.security && a.wps == b.wps;
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

ConnectKnownItem::ConnectKnownItem(WIFI_network n, bool& dirty)
    : MenuItem(ListItemType::Button, "Connect", "Connect to this network.", [&](AbstractMenuItem &item) -> InputReactionHint{
        ScopedOverlay overlay("Connecting...");
        WIFI_connect(net.ssid, net.security); 
        dirty = true;
        return Exit;
    }), net(n)
{}

ConnectNewItem::ConnectNewItem(WIFI_network n, bool& dirty)
    : MenuItem(ListItemType::Button, "Enter WiFi passcode", "Connect to this network.", DeferToSubmenu, new KeyboardPrompt("Enter Wifi passcode", 
        [&](AbstractMenuItem &item) -> InputReactionHint {
            ScopedOverlay overlay("Connecting...");
            WIFI_connectPass(net.ssid, net.security, item.getName().c_str()); 
            dirty = true;
            return Exit; 
        }, true)), net(n)
{}

ForgetItem::ForgetItem(WIFI_network n, bool& dirty)
    : MenuItem(ListItemType::Button, "Forget", "Removes credentials for this network.",
        [&](AbstractMenuItem &item) -> InputReactionHint { 
            WIFI_forget(net.ssid, net.security); 
            dirty = true; 
            return Exit;
        }), net(n)
{}


NetworkItem::NetworkItem(WIFI_network n, bool connected, MenuList* submenu)
    : MenuItem(ListItemType::Custom, n.ssid, n.bssid, DeferToSubmenu, submenu), net(n), connected(connected)
{}

void NetworkItem::drawCustomItem(SDL_Surface *surface, const SDL_Rect &dst, const AbstractMenuItem &item, bool selected) const
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

    // wifi icon
    auto asset =
        net.rssi >= -60 ? ASSET_WIFI :    // anything above 61
        net.rssi >= -70 ? ASSET_WIFI_MED  // -61 and below
                        : ASSET_WIFI_LOW; // -71 and below
    SDL_Rect rect = {0, 0, 12, 12};
    int ix = dst.x + dst.w - SCALE1(OPTION_PADDING + rect.w);
    int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
    SDL_Rect tgt{ix, y};
    GFX_blitAssetColor(asset, NULL, surface, &tgt, THEME_COLOR6);

    // connected
    if(connected) {
        SDL_Rect rect = {0, 0, 12, 12};
        ix = ix - SCALE1(OPTION_PADDING + rect.w);
        int y = dst.y + SCALE1(BUTTON_SIZE - rect.h) / 2;
        SDL_Rect tgt{ix, y};
        GFX_blitAssetColor(ASSET_CHECKCIRCLE, NULL, surface, &tgt, THEME_COLOR6);
    }
    // encrypted
    else if(net.security != SECURITY_NONE) {
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