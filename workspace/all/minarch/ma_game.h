#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void Game_open(char* path);
void Game_close(void);
void Game_changeDisc(char* path);
int extract_zip(char** extensions);

#ifdef __cplusplus
}
#endif
