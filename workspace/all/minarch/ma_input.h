#pragma once

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

int setFastForward(int enable);
void input_poll_callback(void);
int16_t input_state_callback(unsigned port, unsigned device, unsigned index, unsigned id);
void Input_init(const struct retro_input_descriptor *vars);

#ifdef __cplusplus
}
#endif

// Menu functions (defined in ma_menu.c, see ma_menu.h)
#include "ma_menu.h"
