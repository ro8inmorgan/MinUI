#ifndef H700_BOOTLOGO_H
#define H700_BOOTLOGO_H

// stock Anbernic boot logo: bootlogo.bmp on the vfat boot-resource partition
#define BOOTLOGO_PARTITION "/dev/mmcblk0p2"
// presets are keyed by panel resolution, not device name
#define BOOTLOGO_RESOLUTION_DIRS 1
// The RG28XX panel is mounted portrait: the UI is rotated onto it by the SDL
// driver, but the bootloader blits bootlogo.bmp panel-native, so the 480x640
// presets are authored 90° CCW and must be rotated CW to preview how they
// will actually appear at boot.
#define BOOTLOGO_PREVIEW_ROTATE_CW (needs_portrait_sdl)

#endif
