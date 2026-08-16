// tg5040

#ifndef PLATFORM_H
#define PLATFORM_H
#include "device.h"

///////////////////////////////

#ifdef SDL
#	include "sdl.h"
#endif

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
#define	BUTTON_POWER	116 // BUTTON_NA
#define	BUTTON_PLUS		BUTTON_NA
#define	BUTTON_MINUS	BUTTON_NA

///////////////////////////////

#define CODE_UP			CODE_NA
#define CODE_DOWN		CODE_NA
#define CODE_LEFT		CODE_NA
#define CODE_RIGHT		CODE_NA

#define CODE_SELECT		CODE_NA
#define CODE_START		CODE_NA

#define CODE_A			CODE_NA
#define CODE_B			CODE_NA
#define CODE_X			CODE_NA
#define CODE_Y			CODE_NA

#define CODE_L1			CODE_NA
#define CODE_R1			CODE_NA
#define CODE_L2			CODE_NA
#define CODE_R2			CODE_NA
#define CODE_L3			CODE_NA
#define CODE_R3			CODE_NA
#define CODE_L4         CODE_NA
#define CODE_R4         CODE_NA

#define CODE_MENU		CODE_NA
#define CODE_MENU_ALT	13
#define CODE_POWER		102

#define CODE_PLUS		128
#define CODE_MINUS		129

///////////////////////////////
						// HATS
#define JOY_UP			JOY_NA
#define JOY_DOWN		JOY_NA
#define JOY_LEFT		JOY_NA
#define JOY_RIGHT		JOY_NA

#define JOY_SELECT		6
#define JOY_START		7

// TODO: these ended up swapped in the first public release of stock :sob:
#define JOY_A			1
#define JOY_B			0
#define JOY_X			3
#define JOY_Y			2

#define JOY_L1			4
#define JOY_R1			5
#define JOY_L2			JOY_NA
#define JOY_R2			JOY_NA
#define JOY_L3			(deviceModel->joy_l3)
#define JOY_R3			(deviceModel->joy_r3)
#define JOY_L4          JOY_NA
#define JOY_R4          JOY_NA

#define JOY_MENU		8
#define JOY_POWER		102
#define JOY_PLUS		(deviceModel->joy_plus)
#define JOY_MINUS		(deviceModel->joy_minus)

///////////////////////////////
// USER-ASSIGNABLE BUTTONS
#define BTN_FN1			BTN_NONE
#define BTN_FN2			BTN_NONE
#define BTN_FN3			BTN_HOME
#define BTN_FN1_NAME	""
#define BTN_FN2_NAME	""
#define BTN_FN3_NAME	"HOME"

///////////////////////////////

#define AXIS_L2			2 // ABSZ
#define AXIS_R2			5 // RABSZ

#define AXIS_LX			0 // ABS_X, -30k (left) to 30k (right)
#define AXIS_LY			1 // ABS_Y, -30k (up) to 30k (down)
#define AXIS_RX			3 // ABS_RX, -30k (left) to 30k (right)
#define AXIS_RY			4 // ABS_RY, -30k (up) to 30k (down)

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

#define FIXED_SCALE	(deviceModel->scale)
#define FIXED_WIDTH		(deviceModel->width)
#define FIXED_HEIGHT	(deviceModel->height)
#define FIXED_BPP		2
#define FIXED_DEPTH		(FIXED_BPP * 8)
#define FIXED_PITCH		(FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE		(FIXED_PITCH * FIXED_HEIGHT)

///////////////////////////////

#define MAIN_ROW_COUNT		(deviceModel->main_row_count)
#define QUICK_SWITCHER_COUNT	(deviceModel->quick_switcher_count)
#define PADDING	(deviceModel->padding)

///////////////////////////////

#define SDCARD_PATH "/mnt/SDCARD"
#define MUTE_VOLUME_RAW 0

// modetest -v -s 147@100:1280x720
#define SCREEN_FPS 62.948

#define MAX_LIGHTS 4

///////////////////////////////

#endif
