#ifndef PATH_HELPERS_H
#define PATH_HELPERS_H

#include <stddef.h>

int path_arguments_fit(int argc, char *const argv[], size_t path_size);
int path_copy(char *out, size_t out_size, const char *in);
int path_format(char *out, size_t out_size, const char *format, ...);
char *path_format_alloc(const char *format, ...);
char *shell_quote_alloc(const char *value);
char *nextui_path_remove_extension(const char *path);
int path_get_emu_name(char *out, size_t out_size, const char *path, const char *roms_path);

#endif
