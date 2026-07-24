// h700

#ifndef PLATFORM_H
#define PLATFORM_H

///////////////////////////////

#ifdef SDL
#	include "sdl.h"
#endif

///////////////////////////////

extern int is_rg28xx;
extern int is_rg34xx;
extern int is_cube;
extern int hdmi_active;
extern int dev_has_lstick;
extern int dev_has_rstick;

///////////////////////////////

#define BUTTON_UP		BUTTON_NA
#define BUTTON_DOWN		BUTTON_NA
#define BUTTON_LEFT		BUTTON_NA
#define BUTTON_RIGHT	BUTTON_NA

#define BUTTON_SELECT	BUTTON_NA
#define BUTTON_START	BUTTON_NA

#define BUTTON_A		BUTTON_NA
#define BUTTON_B		BUTTON_NA
#define BUTTON_X		BUTTON_NA
#define BUTTON_Y		BUTTON_NA

#define BUTTON_L1		BUTTON_NA
#define BUTTON_R1		BUTTON_NA
#define BUTTON_L2		BUTTON_NA
#define BUTTON_R2		BUTTON_NA
#define BUTTON_L3		BUTTON_NA
#define BUTTON_R3		BUTTON_NA
#define BUTTON_L4		BUTTON_NA
#define BUTTON_R4		BUTTON_NA

#define BUTTON_MENU		BUTTON_NA
#define BUTTON_MENU_ALT	BUTTON_NA
#define	BUTTON_POWER	116
#define	BUTTON_PLUS		BUTTON_NA
#define	BUTTON_MINUS	BUTTON_NA

///////////////////////////////

#define CODE_UP			103
#define CODE_DOWN		108
#define CODE_LEFT		105
#define CODE_RIGHT		106

#define CODE_SELECT		310
#define CODE_START		311

#define CODE_A			304
#define CODE_B			305
#define CODE_X			307
#define CODE_Y			306

#define CODE_L1			308
#define CODE_R1			309
#define CODE_L2			314
#define CODE_R2			315
#define CODE_L4			CODE_NA
#define CODE_R4			CODE_NA
#define CODE_L3			(dev_has_lstick ? 313 : CODE_NA)
#define CODE_R3			(dev_has_rstick ? 316 : CODE_NA)

#define CODE_MENU		312
#define CODE_MENU_ALT	354
#define CODE_POWER		116

#define CODE_PLUS		115
#define CODE_MINUS		114

///////////////////////////////
						// HATS
#define JOY_UP			JOY_NA
#define JOY_DOWN		JOY_NA
#define JOY_LEFT		JOY_NA
#define JOY_RIGHT		JOY_NA

#define JOY_SELECT		6
#define JOY_START		7

#define JOY_A			0
#define JOY_B			1
#define JOY_X			3
#define JOY_Y			2

#define JOY_L1			4
#define JOY_R1			5
#define JOY_L2			10
#define JOY_R2			11
#define JOY_L4			JOY_NA
#define JOY_R4			JOY_NA
#define JOY_L3			(dev_has_lstick ? 9 : JOY_NA)
#define JOY_R3			(dev_has_rstick ? 12 : JOY_NA)

#define JOY_MENU		8
#define JOY_POWER		JOY_NA
#define JOY_PLUS		16
#define JOY_MINUS		15

///////////////////////////////
// USER-ASSIGNABLE BUTTONS
// H700 devices have no dedicated FN1/FN2/HOME buttons for pak launch actions.
#define BTN_FN1			BTN_NONE
#define BTN_FN2			BTN_NONE
#define BTN_FN3			BTN_NONE
#define BTN_FN1_NAME	""
#define BTN_FN2_NAME	""
#define BTN_FN3_NAME	""

///////////////////////////////

#define AXIS_L2			AXIS_NA
#define AXIS_R2			AXIS_NA

#define AXIS_LX			(dev_has_lstick ? 0 : AXIS_NA)
#define AXIS_LY			(dev_has_lstick ? 1 : AXIS_NA)
#define AXIS_RX			(dev_has_rstick ? 2 : AXIS_NA)
#define AXIS_RY			(dev_has_rstick ? 3 : AXIS_NA)

///////////////////////////////

#define BTN_RESUME			BTN_X
#define BTN_SLEEP 			BTN_POWER
#define BTN_WAKE 			BTN_POWER
#define BTN_MOD_VOLUME 		BTN_NONE
#define BTN_MOD_BRIGHTNESS 	BTN_MENU
#define BTN_MOD_COLORTEMP 	BTN_SELECT
#define BTN_MOD_PLUS 		BTN_PLUS
#define BTN_MOD_MINUS 		BTN_MINUS

///////////////////////////////

// While an HDMI cable is connected the whole app runs at 1280x720 (the fb is
// hardware-scaled to a 1080p60 signal by the display engine — see SetHDMI in
// libmsettings). hdmi_active is latched once per process in PLAT_initPlatform,
// so like is_cube these are constant for the process lifetime; the existing
// hotplug quit-and-relaunch plumbing restarts apps on cable changes.
// Values must match HDMI_LOGICAL_* in libmsettings/msettings.c.
#define HAS_HDMI		1
#define HDMI_WIDTH		1280
#define HDMI_HEIGHT		720
#define HDMI_PITCH		(HDMI_WIDTH * FIXED_BPP)
#define HDMI_SIZE		(HDMI_PITCH * HDMI_HEIGHT)

#define FIXED_SCALE 	2
#define FIXED_WIDTH		(hdmi_active?HDMI_WIDTH:(is_cube?720:(is_rg34xx?720:640)))
#define FIXED_HEIGHT	(hdmi_active?HDMI_HEIGHT:(is_cube?720:480))
#define FIXED_BPP		2
#define FIXED_DEPTH		(FIXED_BPP * 8)
#define FIXED_PITCH		(FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE		(FIXED_PITCH * FIXED_HEIGHT)

///////////////////////////////

// Rows that fit above the button hints: (FIXED_HEIGHT/FIXED_SCALE - 2*PADDING - PILL_SIZE) / PILL_SIZE
// 720-tall at scale 2 (HDMI and the cube's square panel) gives 10, like tg5050; 480 gives 6.
#define MAIN_ROW_COUNT ((hdmi_active||is_cube)?10:6)
#define QUICK_SWITCHER_COUNT 3
#define PADDING 5

///////////////////////////////

#define SDCARD_PATH "/mnt/SDCARD"
#define MUTE_VOLUME_RAW 0

#define SCREEN_FPS 60.0
#define MAX_LIGHTS 0

// stock Anbernic boot logo: bootlogo.bmp on the vfat boot-resource partition
#define BOOTLOGO_PARTITION "/dev/mmcblk0p2"
// presets are keyed by panel resolution, not device name
#define BOOTLOGO_RESOLUTION_DIRS 1
// The RG28XX panel is mounted portrait: the UI is rotated onto it by the SDL
// driver, but the bootloader blits bootlogo.bmp panel-native, so the 480x640
// presets are authored 90° CCW and must be rotated CW to preview how they
// will actually appear at boot.
#define BOOTLOGO_PREVIEW_ROTATE_CW (is_rg28xx)

///////////////////////////////

#endif
