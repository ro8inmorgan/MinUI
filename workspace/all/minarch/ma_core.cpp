#include <dlfcn.h>
#include <libgen.h>
#include <string.h>
#include <exception>

#include "ma_internal.h"
#include "ma_saves.h"
#include "ma_video.h"
#include "ma_audio.h"
#include "ma_input.h"
#include "ma_cheats.h"
#include "ma_core.h"


void Core_getName(char* in_name, char* out_name, size_t out_size) {
	snprintf(out_name, out_size, "%s", basename(in_name));
	char* tmp = strrchr(out_name, '_');
	tmp[0] = '\0';
}
void Core_open(const char* core_path, const char* tag_name) {
	LOG_info("Core_open\n");
	core.handle = dlopen(core_path, RTLD_LAZY);
	
	if (!core.handle) LOG_error("%s\n", dlerror());
	
	core.init = (decltype(core.init))dlsym(core.handle, "retro_init");
	core.deinit = (decltype(core.deinit))dlsym(core.handle, "retro_deinit");
	core.get_system_info = (decltype(core.get_system_info))dlsym(core.handle, "retro_get_system_info");
	core.get_system_av_info = (decltype(core.get_system_av_info))dlsym(core.handle, "retro_get_system_av_info");
	core.set_controller_port_device = (decltype(core.set_controller_port_device))dlsym(core.handle, "retro_set_controller_port_device");
	core.reset = (decltype(core.reset))dlsym(core.handle, "retro_reset");
	core.run = (decltype(core.run))dlsym(core.handle, "retro_run");
	core.serialize_size = (decltype(core.serialize_size))dlsym(core.handle, "retro_serialize_size");
	core.serialize = (decltype(core.serialize))dlsym(core.handle, "retro_serialize");
	core.unserialize = (decltype(core.unserialize))dlsym(core.handle, "retro_unserialize");
	core.cheat_reset = (decltype(core.cheat_reset))dlsym(core.handle, "retro_cheat_reset");
	core.cheat_set = (decltype(core.cheat_set))dlsym(core.handle, "retro_cheat_set");
	core.load_game = (decltype(core.load_game))dlsym(core.handle, "retro_load_game");
	core.load_game_special = (decltype(core.load_game_special))dlsym(core.handle, "retro_load_game_special");
	core.unload_game = (decltype(core.unload_game))dlsym(core.handle, "retro_unload_game");
	core.get_region = (decltype(core.get_region))dlsym(core.handle, "retro_get_region");
	core.get_memory_data = (decltype(core.get_memory_data))dlsym(core.handle, "retro_get_memory_data");
	core.get_memory_size = (decltype(core.get_memory_size))dlsym(core.handle, "retro_get_memory_size");
	
	void (*set_environment_callback)(retro_environment_t);
	void (*set_video_refresh_callback)(retro_video_refresh_t);
	void (*set_audio_sample_callback)(retro_audio_sample_t);
	void (*set_audio_sample_batch_callback)(retro_audio_sample_batch_t);
	void (*set_input_poll_callback)(retro_input_poll_t);
	void (*set_input_state_callback)(retro_input_state_t);
	
	set_environment_callback = (decltype(set_environment_callback))dlsym(core.handle, "retro_set_environment");
	set_video_refresh_callback = (decltype(set_video_refresh_callback))dlsym(core.handle, "retro_set_video_refresh");
	set_audio_sample_callback = (decltype(set_audio_sample_callback))dlsym(core.handle, "retro_set_audio_sample");
	set_audio_sample_batch_callback = (decltype(set_audio_sample_batch_callback))dlsym(core.handle, "retro_set_audio_sample_batch");
	set_input_poll_callback = (decltype(set_input_poll_callback))dlsym(core.handle, "retro_set_input_poll");
	set_input_state_callback = (decltype(set_input_state_callback))dlsym(core.handle, "retro_set_input_state");
	
	struct retro_system_info info = {};
	core.get_system_info(&info);
	

	LOG_info("Block Extract: %d\n", info.block_extract);

	Core_getName((char*)core_path, (char*)core.name, sizeof(core.name));
	snprintf((char*)core.version, sizeof(core.version), "%s (%s)", info.library_name, info.library_version);
	snprintf((char*)core.tag, sizeof(core.tag), "%s", tag_name);
	snprintf((char*)core.extensions, sizeof(core.extensions), "%s", info.valid_extensions);
	
	core.need_fullpath = info.need_fullpath;
	
	LOG_info("core: %s version: %s tag: %s (valid_extensions: %s need_fullpath: %i)\n", core.name, core.version, core.tag, info.valid_extensions, info.need_fullpath);
	
	snprintf((char*)core.config_dir, sizeof(core.config_dir), USERDATA_PATH "/%s-%s", core.tag, core.name);
	snprintf((char*)core.states_dir, sizeof(core.states_dir), SHARED_USERDATA_PATH "/%s-%s", core.tag, core.name);
	snprintf((char*)core.saves_dir, sizeof(core.saves_dir), SDCARD_PATH "/Saves/%s", core.tag);
	snprintf((char*)core.bios_dir, sizeof(core.bios_dir), SDCARD_PATH "/Bios/%s", core.tag);
	snprintf((char*)core.cheats_dir, sizeof(core.cheats_dir), SDCARD_PATH "/Cheats/%s", core.tag);
	snprintf((char*)core.overlays_dir, sizeof(core.overlays_dir), SDCARD_PATH "/Overlays/%s", core.tag);
	
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"; mkdir -p \"%s\"", core.config_dir, core.states_dir);
	system(cmd);

	set_environment_callback(environment_callback);
	set_video_refresh_callback(video_refresh_callback);
	set_audio_sample_callback(audio_sample_callback);
	set_audio_sample_batch_callback(audio_sample_batch_callback);
	set_input_poll_callback(input_poll_callback);
	set_input_state_callback(input_state_callback);
}
void Core_init(void) {
	LOG_info("Core_init\n");
	core.init();
	core.initialized = 1;
}

void Core_applyCheats(struct Cheats *cheats)
{
	if (!cheats)
		return;

	if (!core.cheat_reset || !core.cheat_set) {
		LOG_info("Core_applyCheats: core does not support cheats (retro_cheat_%s unavailable)\n",
			!core.cheat_set ? "set" : "reset");
		return;
	}

	// libretro is a C ABI, but some cores (notably Mednafen/supafaust) throw a C++
	// exception out of retro_cheat_set when handed a cheat code they can't parse
	// (e.g. RetroArch Game Genie format "CB01-54DF+..." on a core that expects raw
	// address/value pairs). An uncaught throw unwinds back across the boundary into
	// the frontend and hits std::terminate -> the whole emulator aborts. Guard each
	// core call so a single bad cheat is skipped and logged, and the game survives.
	try {
		core.cheat_reset();
	} catch (const std::exception& e) {
		LOG_error("Core_applyCheats: cheat_reset threw: %s\n", e.what());
		return;
	} catch (...) {
		LOG_error("Core_applyCheats: cheat_reset threw an unknown exception\n");
		return;
	}

	for (int i = 0; i < cheats->count; i++) {
		// no code means it describes a memory write, applied by Cheats_apply
		if (!cheats->cheats[i].enabled || !cheats->cheats[i].code)
			continue;
		const char* name = cheats->cheats[i].name ? cheats->cheats[i].name : "";
		const char* code = cheats->cheats[i].code ? cheats->cheats[i].code : "";
		try {
			// Most cores parse the code here; some (Mednafen/supafaust) defer to
			// retro_run() — that deferred throw is caught in run_frame().
			core.cheat_set(i, cheats->cheats[i].enabled, cheats->cheats[i].code);
		} catch (const std::exception& e) {
			LOG_error("Core_applyCheats: core rejected cheat %d \"%s\" (%s): %s\n", i, name, code, e.what());
		} catch (...) {
			LOG_error("Core_applyCheats: core rejected cheat %d \"%s\" (%s): unknown exception\n", i, name, code);
		}
	}
}

int Core_updateAVInfo(void) {
	struct retro_system_av_info av_info = {};
	core.get_system_av_info(&av_info);

	double a = av_info.geometry.aspect_ratio;
	if (a<=0) a = (double)av_info.geometry.base_width / av_info.geometry.base_height;

	int changed = (core.fps != av_info.timing.fps || core.sample_rate != av_info.timing.sample_rate || core.aspect_ratio != a);

	core.fps = av_info.timing.fps;
	core.sample_rate = av_info.timing.sample_rate;
	core.aspect_ratio = a;

	if (changed) LOG_info("aspect_ratio: %f (%ix%i) fps: %f\n", a, av_info.geometry.base_width,av_info.geometry.base_height, core.fps);

	return changed;
}

void Core_load(void) {
	LOG_info("Core_load\n");
	struct retro_game_info game_info;
	game_info.path = game.tmp_path[0]?game.tmp_path:game.path;
	game_info.data = game.data;
	game_info.size = game.size;
	LOG_info("game path: %s (%i)\n", game_info.path, game.size);
	try {
		core.load_game(&game_info);
	} catch (const std::exception& e) {
		LOG_error("Core_load: retro_load_game threw: %s\n", e.what());
		exit(1);
	} catch (...) {
		LOG_error("Core_load: retro_load_game threw an unknown exception\n");
		exit(1);
	}

	if (Cheats_load())
		Core_applyCheats(&cheatcodes);

	SRAM_read();
	RTC_read();
	// NOTE: must be called after core.load_game!
	core.set_controller_port_device(0, RETRO_DEVICE_JOYPAD); // set a default, may update after loading configs
	Core_updateAVInfo();
}
void Core_reset(void) {
	core.reset();
	Rewind_on_state_change();
}
void Core_unload(void) {
	// Disabling this is a dumb hack for bluetooth, we should really be using 
	// bluealsa with --keep-alive=-1 - but SDL wont reconnect the stream on next start.
	// Reenable as soon as we have a more recent SDL available, if ever.
	//SND_quit();
}
void Core_quit(void) {
	if (core.initialized) {
		SRAM_write();
		Cheats_free();
		RTC_write();
		core.unload_game();
		core.deinit();
		core.initialized = 0;
	}
}
void Core_close(void) {
	if (core.handle) dlclose(core.handle);
}
