#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <msettings.h>

#include "defines.h"
#include "api.h"
#include "shader_sets.h"

#define SET_ROW_PADDING 8

static volatile sig_atomic_t quit;

static void sigHandler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
		quit = 1;
}

static void renderTitle(SDL_Surface *screen, int reserved_width)
{
	int max_width = screen->w - SCALE1(PADDING * 2) - reserved_width;
	char title[256];
	int text_width = GFX_truncateText(font.large, "Shader Sets", title,
		max_width, SCALE1(BUTTON_PADDING * 2));
	SDL_Surface *text;

	max_width = MIN(max_width, text_width);
	text = TTF_RenderUTF8_Blended(font.large, title, COLOR_WHITE);
	GFX_blitPill(ASSET_BLACK_PILL, screen,
		&(SDL_Rect){SCALE1(PADDING), SCALE1(PADDING), max_width, SCALE1(PILL_SIZE)});
	SDL_BlitSurface(text,
		&(SDL_Rect){0, 0, max_width - SCALE1(BUTTON_PADDING * 2), text->h},
		screen,
		&(SDL_Rect){SCALE1(PADDING + BUTTON_PADDING), SCALE1(PADDING + 4)});
	SDL_FreeSurface(text);
}

static void renderSelection(SDL_Surface *screen, const char *name)
{
	int width = screen->w - SCALE1(PADDING * 2);
	int y = (screen->h - SCALE1(BUTTON_SIZE)) / 2;
	const char *label = "Shader set";
	const char *value = ShaderSets_displayName(name);
	char display_value[256];
	SDL_Surface *label_text;
	SDL_Surface *value_text;

	GFX_truncateText(font.tiny, value, display_value,
		width / 2, SCALE1(SET_ROW_PADDING));
	GFX_blitPillLight(ASSET_BUTTON, screen,
		&(SDL_Rect){SCALE1(PADDING), y, width, SCALE1(BUTTON_SIZE)});

	label_text = TTF_RenderUTF8_Blended(font.small, label, COLOR_BLACK);
	value_text = TTF_RenderUTF8_Blended(font.tiny, display_value, COLOR_BLACK);

	SDL_BlitSurface(label_text, NULL, screen,
		&(SDL_Rect){
			SCALE1(PADDING + SET_ROW_PADDING),
			y + (SCALE1(BUTTON_SIZE) - label_text->h) / 2
		});
	SDL_BlitSurface(value_text, NULL, screen,
		&(SDL_Rect){
			screen->w - SCALE1(PADDING + SET_ROW_PADDING) - value_text->w,
			y + (SCALE1(BUTTON_SIZE) - value_text->h) / 2
		});

	SDL_FreeSurface(label_text);
	SDL_FreeSurface(value_text);
}

int main(int argc, char *argv[])
{
	ShaderSetList list;
	SDL_Surface *screen;
	int selected;
	int dirty = 1;
	int show_setting = 0;
	int was_online;
	int had_bt;
	bool save_error = false;

	(void)argc;
	(void)argv;

	InitSettings();
	PWR_setCPUSpeed(CPU_SPEED_AUTO);
	screen = GFX_init(MODE_MAIN);
	PAD_init();
	PWR_init();
	VIB_init();

	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	if (!ShaderSets_list(&list)) {
		LOG_error("Unable to enumerate shader sets\n");
		QuitSettings();
		VIB_quit();
		PWR_quit();
		PAD_quit();
		GFX_quit();
		return EXIT_FAILURE;
	}

	selected = ShaderSets_activeIndex(&list);
	was_online = PWR_isOnline();
	had_bt = PLAT_btIsConnected();

	while (!quit) {
		GFX_startFrame();
		PAD_poll();

		if (PAD_justRepeated(BTN_LEFT)) {
			selected = (selected - 1 + list.count) % list.count;
			save_error = false;
			dirty = 1;
		}
		else if (PAD_justRepeated(BTN_RIGHT)) {
			selected = (selected + 1) % list.count;
			save_error = false;
			dirty = 1;
		}
		else if (PAD_justPressed(BTN_A)) {
			if (ShaderSets_setActive(list.names[selected]))
				quit = 1;
			else {
				LOG_error("Unable to save shader set selection\n");
				if (CFG_getHaptics())
					VIB_triplePulse(5, 150, 200);
				save_error = true;
				dirty = 1;
			}
		}
		else if (PAD_justPressed(BTN_B)) {
			quit = 1;
		}

		PWR_update(&dirty, &show_setting, NULL, NULL);

		int is_online = PWR_isOnline();
		if (was_online != is_online)
			dirty = 1;
		was_online = is_online;

		int has_bt = PLAT_btIsConnected();
		if (had_bt != has_bt)
			dirty = 1;
		had_bt = has_bt;

		if (dirty) {
			GFX_clear(screen);
			int reserved_width = GFX_blitHardwareGroup(screen, show_setting);

			renderTitle(screen, reserved_width);
			renderSelection(screen, list.names[selected]);

			if (save_error) {
				GFX_blitWrappedText(font.tiny, "Could not save the selected shader set.",
					screen->w - SCALE1(PADDING * 2), 2, COLOR_LIGHT_TEXT,
					screen, SCALE1(PADDING + PILL_SIZE + BUTTON_MARGIN));
			}
			else if (list.count == 1) {
				GFX_blitWrappedText(font.tiny,
					"No set configs found in /Shaders/sets.",
					screen->w - SCALE1(PADDING * 2), 2, COLOR_LIGHT_TEXT,
					screen, SCALE1(PADDING + PILL_SIZE + BUTTON_MARGIN));
			}

			if (show_setting)
				GFX_blitHardwareHints(screen, show_setting);
			else
				GFX_blitButtonGroup((char *[]){"L/R", "CHOOSE", NULL}, 0, screen, 0);
			GFX_blitButtonGroup((char *[]){"B", "CANCEL", "A", "APPLY", NULL}, 1, screen, 1);

			GFX_flip(screen);
			dirty = 0;
		}
		else {
			GFX_sync();
		}
	}

	ShaderSets_freeList(&list);
	QuitSettings();
	VIB_quit();
	PWR_quit();
	PAD_quit();
	GFX_quit();
	return EXIT_SUCCESS;
}
