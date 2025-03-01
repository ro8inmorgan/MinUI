// v10/platform/platform.h

#ifndef PLATFORM_H
#define PLATFORM_H

///////////////////////////////

#include "sdl.h"

///////////////////////////////

#define BUTTON_UP BUTTON_NA
#define BUTTON_DOWN BUTTON_NA
#define BUTTON_LEFT BUTTON_NA
#define BUTTON_RIGHT BUTTON_NA

#define BUTTON_SELECT BUTTON_NA
#define BUTTON_START BUTTON_NA

#define BUTTON_A BUTTON_NA
#define BUTTON_B BUTTON_NA
#define BUTTON_X BUTTON_NA
#define BUTTON_Y BUTTON_NA

#define BUTTON_L1 BUTTON_NA
#define BUTTON_R1 BUTTON_NA
#define BUTTON_L2 BUTTON_NA
#define BUTTON_R2 BUTTON_NA
#define BUTTON_L3 BUTTON_NA
#define BUTTON_R3 BUTTON_NA

#define BUTTON_MENU BUTTON_NA
#define BUTTON_POWER BUTTON_NA
#define BUTTON_PLUS BUTTON_NA
#define BUTTON_MINUS BUTTON_NA

///////////////////////////////

#define CODE_UP CODE_NA
#define CODE_DOWN CODE_NA
#define CODE_LEFT CODE_NA
#define CODE_RIGHT CODE_NA

#define CODE_SELECT CODE_NA
#define CODE_START CODE_NA

#define CODE_A CODE_NA
#define CODE_B CODE_NA
#define CODE_X CODE_NA
#define CODE_Y CODE_NA

#define CODE_L1 CODE_NA
#define CODE_R1 CODE_NA
#define CODE_L2 CODE_NA
#define CODE_R2 CODE_NA
#define CODE_L3 CODE_NA
#define CODE_R3 CODE_NA

#define CODE_MENU CODE_NA
#define CODE_POWER 116

#define CODE_PLUS 115
#define CODE_MINUS 114

///////////////////////////////

// TODO: unused
#define JOY_UP 13
#define JOY_DOWN 16
#define JOY_LEFT 14
#define JOY_RIGHT 15

#define JOY_SELECT 6
#define JOY_START 7

#define JOY_A 0
#define JOY_B 1
#define JOY_X 3
#define JOY_Y 2

#define JOY_L1 4
#define JOY_R1 5
#define JOY_L2 9
#define JOY_R2 10
#define JOY_L3 JOY_NA
#define JOY_R3 JOY_NA

#define JOY_MENU 8
#define JOY_POWER JOY_NA
#define JOY_PLUS JOY_NA
#define JOY_MINUS JOY_NA

///////////////////////////////

#define BTN_RESUME BTN_X
#define BTN_SLEEP BTN_POWER
#define BTN_WAKE BTN_POWER
#define BTN_MOD_VOLUME BTN_START
#define BTN_MOD_BRIGHTNESS BTN_MENU
#define BTN_MOD_COLORTEMP BTN_SELECT
#define BTN_MOD_PLUS BTN_PLUS
#define BTN_MOD_MINUS BTN_MINUS

///////////////////////////////

#define FIXED_SCALE 1
#define FIXED_WIDTH 480
#define FIXED_HEIGHT 320
#define FIXED_BPP 2
#define FIXED_DEPTH (FIXED_BPP * 8)
#define FIXED_PITCH (FIXED_WIDTH * FIXED_BPP)
#define FIXED_SIZE (FIXED_PITCH * FIXED_HEIGHT)

///////////////////////////////

#define MAIN_ROW_COUNT 8
#define PADDING 20

///////////////////////////////

#define SDCARD_PATH "/storage"
#define MUTE_VOLUME_RAW 0
#define HAS_NEON
#define SAMPLES 400 // fix for (most) fceumm underruns, not super helpful here

///////////////////////////////

#endif
