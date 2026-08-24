#include "path_helpers.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int path_arguments_fit(int argc, char *const argv[], size_t path_size) {
    return argc >= 3 && argv && argv[1] && argv[2] &&
           strlen(argv[1]) < path_size && strlen(argv[2]) < path_size;
}

int path_copy(char *out, size_t out_size, const char *in) {
    size_t len;

    if (!out || !out_size || !in) return -1;
    len = strlen(in);
    if (len >= out_size) {
        out[0] = '\0';
        return -1;
    }
    memcpy(out, in, len + 1);
    return 0;
}

int path_format(char *out, size_t out_size, const char *format, ...) {
    va_list args;
    int written;

    if (!out || !out_size || !format) return -1;
    va_start(args, format);
    written = vsnprintf(out, out_size, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= out_size) {
        out[0] = '\0';
        return -1;
    }
    return 0;
}

char *path_format_alloc(const char *format, ...) {
    va_list args;
    va_list copy;
    int written;
    char *out;

    if (!format) return NULL;
    va_start(args, format);
    va_copy(copy, args);
    written = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (written < 0) {
        va_end(args);
        return NULL;
    }
    out = malloc((size_t)written + 1);
    if (!out) {
        va_end(args);
        return NULL;
    }
    vsnprintf(out, (size_t)written + 1, format, args);
    va_end(args);
    return out;
}

char *shell_quote_alloc(const char *value) {
    size_t len;
    size_t quotes = 0;
    char *out;
    char *p;

    if (!value) value = "";
    len = strlen(value);
    for (size_t i = 0; i < len; i++) {
        if (value[i] == '\'') quotes++;
    }
    if (quotes > (SIZE_MAX - len - 3) / 4) return NULL;
    out = malloc(len + quotes * 4 + 3);
    if (!out) return NULL;

    p = out;
    *p++ = '\'';
    for (size_t i = 0; i < len; i++) {
        if (value[i] == '\'') {
            *p++ = '\'';
            *p++ = '"';
            *p++ = '\'';
            *p++ = '"';
            *p++ = '\'';
        } else {
            *p++ = value[i];
        }
    }
    *p++ = '\'';
    *p = '\0';
    return out;
}

char *nextui_path_remove_extension(const char *path) {
    char *out;
    char *dot;

    if (!path) return NULL;
    out = strdup(path);
    if (!out) return NULL;
    dot = strrchr(out, '.');
    if (dot && dot[1] && dot[1] != ' ' && dot[2]) *dot = '\0';
    return out;
}

int path_get_emu_name(char *out, size_t out_size, const char *path, const char *roms_path) {
    const char *name = path;
    const char *slash;
    const char *open;
    const char *close;
    size_t len;

    if (!out || !out_size || !path || !roms_path) return -1;
    if (strncasecmp(path, roms_path, strlen(roms_path)) == 0 &&
        path[strlen(roms_path)] == '/') {
        name = path + strlen(roms_path) + 1;
        slash = strchr(name, '/');
        len = slash ? (size_t)(slash - name) : strlen(name);
    } else {
        len = strlen(name);
    }
    if (len >= out_size) {
        out[0] = '\0';
        return -1;
    }
    memcpy(out, name, len);
    out[len] = '\0';

    open = strrchr(out, '(');
    if (!open) return 0;
    close = strchr(open + 1, ')');
    if (!close) return 0;
    len = (size_t)(close - open - 1);
    memmove(out, open + 1, len);
    out[len] = '\0';
    return 0;
}
