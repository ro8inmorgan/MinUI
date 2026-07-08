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
#define CODE_L3			313
#define CODE_R3			316

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
#define JOY_L2			9
#define JOY_R2			10
#define JOY_L3			(is_rg34xx || is_rg28xx ? JOY_NA : 11)
#define JOY_R3			(is_rg34xx || is_rg28xx ? JOY_NA : 12)

#define JOY_MENU		8
#define JOY_POWER		JOY_NA
#define JOY_PLUS		18
#define JOY_MINUS		17

///////////////////////////////

#define AXIS_L2			AXIS_NA
#define AXIS_R2			AXIS_NA

#define AXIS_LX			0
#define AXIS_LY			1
#define AXIS_RX			2
#define AXIS_RY			3

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

#define FIXED_SCALE 	2
#define FIXED_WIDTH		(is_cube?720:(is_rg34xx?720:640))
#define FIXED_HEIGHT	(is_cube?720:480)
#define FIXED_BPP		2
#define FIXED_DEPTH		(FIXED_BPP * 8)
#define FIXED_PITCH		(FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE		(FIXED_PITCH * FIXED_HEIGHT)

///////////////////////////////

#define MAIN_ROW_COUNT (is_cube?8:6)
#define QUICK_SWITCHER_COUNT 3
#define PADDING 5

///////////////////////////////

#define SDCARD_PATH "/mnt/SDCARD"
#define MUTE_VOLUME_RAW 0

#define SCREEN_FPS 60.0
#define MAX_LIGHTS 0

///////////////////////////////

#endif
