#include <SDL2/SDL.h>
#include <exception>

#include "ma_internal.h"
#include "ma_rewind.h"
#include "ma_input.h"
#include "ma_config.h"
#include "ma_cheats.h"
#include "ma_runframe.h"
#include "notification.h"

// Recover after a core throws mid-emulation (see run_frame). The known trigger is
// a cheat the core can't parse (Mednafen/supafaust defers Game-Genie parsing to
// retro_run and throws). Clear the core's cheat state so the next frame is clean,
// clear the frontend's enabled flags so the cheat menu reflects reality, and tell
// the user once. Called only from the catch, so it runs at most once per fault.
static void recoverFromCoreFault(void) {
	if (core.cheat_reset) { try { core.cheat_reset(); } catch (...) {} }
	for (size_t i = 0; i < cheatcodes.count; i++) cheatcodes.cheats[i].enabled = 0;
	cheatcodes.enabled = 0;
	Notification_push(NOTIFICATION_SETTING, "Cheat rejected by core - cheats disabled", NULL);
}

void chooseSyncRef(void) {
	switch (sync_ref) {
		case SYNC_SRC_AUTO:   use_core_fps = (core.get_region() == RETRO_REGION_PAL); break;
		case SYNC_SRC_SCREEN: use_core_fps = 0; break;
		case SYNC_SRC_CORE:   use_core_fps = 1; break;
	}
	LOG_info("%s: sync_ref is set to %s, game region is %s, use core fps = %s\n",
		  __FUNCTION__,
		  sync_ref_labels[sync_ref],
		  core.get_region() == RETRO_REGION_NTSC ? "NTSC" : "PAL",
		  use_core_fps ? "yes" : "no");
}

static void limitFF(void) {
	static uint64_t ff_frame_time = 0;
	static uint64_t last_time = 0;
	static int last_max_speed = -1;
	if (last_max_speed!=max_ff_speed) {
		last_max_speed = max_ff_speed;
		ff_frame_time = 1000000 / (core.fps * (max_ff_speed + 1));
	}

	uint64_t now = getMicroseconds();
	if (ff.active && max_ff_speed) {
		if (last_time == 0) last_time = now;
		int elapsed = now - last_time;
		if (elapsed>0 && elapsed<0x80000) {
			if (elapsed<ff_frame_time) {
				int delay = (ff_frame_time - elapsed) / 1000;
				if (delay>0 && delay<17) { // don't allow a delay any greater than a frame
					SDL_Delay(delay);
				}
			}
			last_time += ff_frame_time;
			return;
		}
	}
	last_time = now;
}

void run_frame(void) {
	// Memory-write cheats have to be rewritten every frame; they're pure frontend
	// RAM pokes, so they stay outside the core guard below.
	Cheats_apply();

	// Some cores (Mednafen/supafaust) DEFER cheat parsing to the emulation frame:
	// retro_cheat_set only stores the code, and retro_run() parses it and throws a
	// C++ exception (Mednafen::MDFN_Error) if it can't (e.g. Game Genie format on a
	// core that wants raw addresses). Uncaught, that unwinds out of core.run() into
	// the frontend and hits std::terminate -> abort. Guard the whole frame so a
	// throwing core is recovered by clearing cheats instead of killing the emulator.
	try {
	// if rewind is toggled, fast-forward toggle must stay off; fast-forward hold pauses rewind
	int do_rewind = (rewind_st.pressed || rewind_st.toggle) && !(rewind_st.toggle && ff.hold_active);
	if (do_rewind) {
		int was_rewinding = rewind_st.active;
		int rewind_result = Rewind_step_back();
		if (rewind_result == REWIND_STEP_OK) {
			// Actually stepped back - run one frame to render the restored state
			rewind_st.active = 1;
			ff.active = 0;
			core.run();
		}
		else if (rewind_result == REWIND_STEP_CADENCE) {
			// Waiting for cadence - don't run core, just re-render current frame
			rewind_st.active = 1;
			ff.active = 0;
			// Poll input manually since core.run() isn't called
			input_poll_callback();
			// Skip core.run() entirely to avoid advancing the game
		}
		else {
			int hold_empty = rewind_ctx.enabled && rewind_st.pressed && !rewind_st.toggle;
			if (hold_empty) {
				// Hold-to-rewind: freeze when empty to avoid advance/rewind oscillation.
				rewind_st.active = was_rewinding ? 1 : 0;
				// Poll input manually so release is detected while core.run() is skipped
				input_poll_callback();
			} else {
				// Buffer empty: auto untoggle rewind, resume FF if it was paused for a hold
				if (rewind_st.toggle) rewind_st.toggle = 0;
				if (ff.paused_by_rewind_hold && ff.toggled) {
					ff.paused_by_rewind_hold = 0;
					ff.active = setFastForward(1);
				}
				if (was_rewinding) {
					rewind_st.active = 1;
					Rewind_sync_encode_state();
				}
				rewind_st.active = 0;
				core.run();
				Rewind_push(0);
			}
		}
	}
	else {
		Rewind_sync_encode_state();
		rewind_st.active = 0;
		if (ff.paused_by_rewind_hold && !rewind_st.pressed) {
			// resume fast forward after hold rewind ends
			if (ff.toggled) ff.active = setFastForward(1);
			ff.paused_by_rewind_hold = 0;
		}

		core.run();
		Rewind_push(0);
	}
	limitFF();
	} catch (const std::exception& e) {
		LOG_error("run_frame: core threw during emulation: %s — disabling cheats to recover\n", e.what());
		recoverFromCoreFault();
	} catch (...) {
		LOG_error("run_frame: core threw an unknown exception during emulation — disabling cheats to recover\n");
		recoverFromCoreFault();
	}
}
