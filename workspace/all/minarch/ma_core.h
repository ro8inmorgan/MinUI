#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void Core_getName(char* in_name, char* out_name, size_t out_size);
void Core_open(const char* core_path, const char* tag_name);
void Core_init(void);
void Core_applyCheats(struct Cheats *cheats);
int  Core_updateAVInfo(void);
void Core_load(void);
void Core_reset(void);
void Core_unload(void);
void Core_quit(void);
void Core_close(void);

#ifdef __cplusplus
}
#endif
