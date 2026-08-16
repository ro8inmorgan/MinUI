#pragma once

#include <cstdint>

#define PALETTE_VERSION_MAX 1
#define PALETTE_NAME_MAX 64   // keep in sync with NextUISettings.paletteName in config.h
#define PALETTE_PATH_MAX 512
#define PALETTE_COLOR_COUNT 7

struct ColorPalette
{
	int version;                    // palette-file format version
	char name[PALETTE_NAME_MAX];    // display label
	char path[PALETTE_PATH_MAX];    // absolute path to the palette file
	bool builtin;                   // true if shipped in RES_PATH/palettes (read-only)
	uint32_t colors[PALETTE_COLOR_COUNT]; // packed RGBA, index 0 == color1
};

int PALETTE_enumerate(ColorPalette *out, int max);
void PALETTE_apply(const ColorPalette *palette);
