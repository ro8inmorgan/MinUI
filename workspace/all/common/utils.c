#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for strcasestr
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/time.h>
#include "defines.h"
#include "utils.h"
#include "path_helpers.h"

///////////////////////////////////////

int prefixMatch(char* pre, const char* str) {
	return (strncasecmp(pre,str,strlen(pre))==0);
}
int suffixMatch(char* suf, const char* str) {
	int len = strlen(suf);
	int offset = strlen(str)-len;
	return (offset>=0 && strncasecmp(suf, str+offset, len)==0);
}
int exactMatch(const char* str1, const char* str2) {
	if (!str1 || !str2) return 0; // NULL isn't safe here
	size_t len1 = strlen(str1);
	if (len1!=strlen(str2)) return 0;
	return (strncmp(str1,str2,len1)==0);
}
int containsString(char* haystack, char* needle) {
	return strcasestr(haystack, needle) != NULL;
}
int hide(char* file_name) {
	return file_name[0]=='.' || suffixMatch(".disabled", file_name) || exactMatch("map.txt", file_name);
}
char *splitString(char *str, const char *delim)
{
    char *p = strstr(str, delim);
    if (p == NULL)
        return NULL;          // delimiter not found
    *p = '\0';                // terminate string after head
    return p + strlen(delim); // return tail substring
}
void truncateString(char *string, size_t max_len) {
	size_t len = strlen(string) + 1;
	if (len <= max_len) return;

	strncpy(&string[max_len - 4], "...\0", 4);
}
void wrapString(char *string, size_t max_len, size_t max_lines) {
	char *line = string;

	for (size_t i = 1; i < max_lines; i++) {
		char *p = line;
		char *prev;
		do {
			prev = p;
			p = strchr(prev+1, ' ');
		} while (p && p - line < (int)max_len);

		if (!p && strlen(line) < max_len) break;

		if (prev && prev != line) {
			line = prev + 1;
			*prev = '\n';
		}
	}
	truncateString(line, max_len);
}
// TODO: verify this yields the same result as the one in minui.c, remove one
// This one does not modify the input, cause we arent savages
char *replaceString2(const char *orig, char *rep, char *with)
{
    const char *ins;     // the next insert point
    char *tmp;     // varies
    int len_rep;   // length of rep (the string to remove)
    int len_with;  // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;     // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (ins = strstr(ins, rep)); ++count)
        ins += len_rep;

    char *result =
        (char *)malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    tmp = result;

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}
// Stores the trimmed input string into the given output buffer, which must be
// large enough to store the result.  If it is too small, the output is
// truncated.
size_t trimString(char *out, size_t len, const char *str, bool first)
{
    if (len == 0)
        return 0;

    const char *end;
    size_t out_size;
    bool is_string = false;

    // Trim leading space
    while (strchr("\r\n\t {},", (unsigned char)*str) != NULL)
        str++;

    end = str + 1;

    if ((unsigned char)*str == '"') {
        is_string = true;
        str++;
        while (strchr("\r\n\"", (unsigned char)*end) == NULL)
            end++;
    }

    if (*str == 0) // All spaces?
    {
        *out = 0;
        return 1;
    }

    // Trim trailing space
    if (first)
        while (strchr("\r\n\t {},", (unsigned char)*end) == NULL)
            end++;
    else {
        end = str + strlen(str) - 1;
        while (end > str && strchr("\r\n\t {},", (unsigned char)*end) != NULL)
            end--;
        end++;
    }

    if (is_string && (unsigned char)*(end - 1) == '"')
        end--;

    // Set output size to minimum of trimmed string length and buffer size minus
    // 1
    size_t trimmed_size = (size_t)(end - str);
    out_size = trimmed_size < len - 1 ? trimmed_size : len - 1;

    // Copy trimmed string and add null terminator
    memcpy(out, str, out_size);
    out[out_size] = 0;

    return out_size;
}

void removeParentheses(char *str_out, const char *str_in)
{
    char temp[STR_MAX];
    int len = strlen(str_in);
    int c = 0;
    bool inside = false;
    char end_char;

    for (int i = 0; i < len && i < STR_MAX; i++) {
        if (!inside && (str_in[i] == '(' || str_in[i] == '[')) {
            end_char = str_in[i] == '(' ? ')' : ']';
            inside = true;
            continue;
        }
        else if (inside) {
            if (str_in[i] == end_char)
                inside = false;
            continue;
        }
        temp[c++] = str_in[i];
    }

    temp[c] = '\0';

    trimString(str_out, STR_MAX - 1, temp, false);
}
void serializeTime(char *dest_str, int nTime)
{
    if (nTime >= 60) {
        int h = nTime / 3600;
        int m = (nTime - 3600 * h) / 60;
        if (h > 0) {
            sprintf(dest_str, "%dh %dm", h, m);
        }
        else {
            sprintf(dest_str, "%dm %ds", m, nTime - 60 * m);
        }
    }
    else {
        sprintf(dest_str, "%ds", nTime);
    }
}
int countChar(const char *str, char ch)
{
    size_t i;
    int count = 0;
    for (i = 0; i <= strlen(str); i++) {
        if (str[i] == ch) {
            count++;
        }
    }
    return count;
}
char *removeExtension(const char *myStr)
{
    return nextui_path_remove_extension(myStr);
}
const char *baseName(const char *filename)
{
    const char *p = strrchr(filename, '/');
    return p ? p + 1 : filename;
}
int folderPathSafe(const char *path, char *result, size_t result_size) {
    if (!path || !result || !result_size) return -1;
    const char *slash = strrchr(path, '/');
    size_t length = slash ? (size_t)(slash - path) : 0;
    if (length >= result_size) { result[0] = '\0'; return -1; }
    memcpy(result, path, length);
    result[length] = '\0';
    return 0;
}

void folderPath(const char *path, char *result) { folderPathSafe(path, result, MAX_PATH); }
void cleanName(char *name_out, const char *file_name)
{
    char *name_without_ext = removeExtension(file_name);
    char *no_underscores = replaceString2(name_without_ext, "_", " ");
    char *dot_ptr = strstr(no_underscores, ".");
    if (dot_ptr != NULL) {
        char *s = no_underscores;
        while (isdigit(*s) && s < dot_ptr)
            s++;
        if (s != dot_ptr)
            dot_ptr = no_underscores;
        else {
            dot_ptr++;
            if (dot_ptr[0] == ' ')
                dot_ptr++;
        }
    }
    else {
        dot_ptr = no_underscores;
    }
    removeParentheses(name_out, dot_ptr);
    free(name_without_ext);
    free(no_underscores);
}
int pathRelativeToSafe(char *path_out, size_t path_out_size, const char *dir_from, const char *file_to)
{
    char *abs_from;
    char *abs_to;
    char *p1;
    char *p2;
    size_t parents;
    size_t suffix_length;

    if (!path_out || !path_out_size) return -1;
    path_out[0] = '\0';
    abs_from = realpath(dir_from, NULL);
    abs_to = realpath(file_to, NULL);
    if (!abs_from || !abs_to) {
        free(abs_from);
        free(abs_to);
        return -1;
    }

    p1 = abs_from;
    p2 = abs_to;
    while (*p1 && *p1 == *p2) ++p1, ++p2;
    if ((*p1 || *p2) && *p2 != '/') {
        while (p1 > abs_from && p1[-1] != '/') --p1, --p2;
    }
    if (*p2 == '/') ++p2;
    parents = *p1 ? (size_t)countChar(p1, '/') + 1 : 0;
    suffix_length = strlen(p2);
    if (parents > (SIZE_MAX - suffix_length) / 3 || parents * 3 + suffix_length >= path_out_size) {
        free(abs_from);
        free(abs_to);
        return -1;
    }
    char *out = path_out;
    for (size_t i = 0; i < parents; i++) { memcpy(out, "../", 3); out += 3; }
    memcpy(out, p2, suffix_length + 1);
    free(abs_from);
    free(abs_to);
    return 0;
}

bool pathRelativeTo(char *path_out, const char *dir_from, const char *file_to) {
    return pathRelativeToSafe(path_out, MAX_PATH, dir_from, file_to) == 0;
}

int getDisplayNameSafe(const char* in_name, char* out_name, size_t out_name_size) {
    char work_name[MAX_PATH], original[MAX_PATH];
    if (!out_name || !out_name_size) return -1;
    out_name[0] = '\0';
    if (path_copy(work_name, sizeof(work_name), in_name) != 0) return -1;
    if (suffixMatch("/" PLATFORM, work_name)) {
        char *slash = strrchr(work_name, '/');
        if (slash) *slash = '\0';
    }
    const char *name = strrchr(work_name, '/');
    if (path_copy(original, sizeof(original), name ? name + 1 : work_name) != 0) return -1;
    char *tmp;
    while ((tmp = strrchr(original, '.')) != NULL) {
        size_t length = strlen(tmp);
        if (length <= 2 || length > 5) break;
        *tmp = '\0';
    }
    while ((tmp = strrchr(original, '(')) != NULL || (tmp = strrchr(original, '[')) != NULL) {
        if (tmp == original) break;
        *tmp = '\0';
    }
    if (!original[0] && path_copy(original, sizeof(original), name ? name + 1 : work_name) != 0) return -1;
    size_t length = strlen(original);
    while (length && isspace((unsigned char)original[length - 1])) original[--length] = '\0';
    return path_copy(out_name, out_name_size, original);
}

void getDisplayName(const char* in_name, char* out_name) { getDisplayNameSafe(in_name, out_name, MAX_PATH); }
int getEmuNameSafe(const char* in_name, char* out_name, size_t out_size) {
	return path_get_emu_name(out_name, out_size, in_name, ROMS_PATH);
}
void getEmuName(const char* in_name, char* out_name) { // NOTE: output must be MAX_PATH bytes
	getEmuNameSafe(in_name, out_name, MAX_PATH);
}
int getEmuPathSafe(const char* emu_name, char* pak_path, size_t pak_path_size) {
	if (path_format(pak_path, pak_path_size, "%s/Emus/%s/%s.pak/launch.sh", SDCARD_PATH, PLATFORM, emu_name) != 0)
		return -1;
	if (exists(pak_path)) return 0;
	return path_format(pak_path, pak_path_size, "%s/Emus/%s.pak/launch.sh", PAKS_PATH, emu_name);
}
void getEmuPath(char* emu_name, char* pak_path) {
	getEmuPathSafe(emu_name, pak_path, MAX_PATH);
}

void normalizeNewline(char* line) {
	int len = strlen(line);
	if (len>1 && line[len-1]=='\n' && line[len-2]=='\r') { // windows!
		line[len-2] = '\n';
		line[len-1] = '\0';
	}
}
void trimTrailingNewlines(char* line) {
	int len = strlen(line);
	while (len>0 && line[len-1]=='\n') {
		line[len-1] = '\0'; // trim newline
		len -= 1;
	}
}
void trimSortingMeta(char** str) { // eg. `001) `
	// TODO: this code is suss
	char* safe = *str;
	while(isdigit(**str)) *str += 1; // ignore leading numbers

	if (*str[0]==')') { // then match a closing parenthesis
		*str += 1;
	}
	else { //  or bail, restoring the string to its original value
		*str = safe;
		return;
	}
	
	while(isblank(**str)) *str += 1; // ignore leading space
}

///////////////////////////////////////

int exists(char* path) {
	return access(path, F_OK)==0;
}
void touch(char* path) {
	close(open(path, O_RDWR|O_CREAT, 0777));
}
int toggle(char *path) {
    if (access(path, F_OK) == 0) {
        unlink(path);
        return 0;
    } else {
        touch(path);
        return 1;
    }
}
void putFile(char* path, char* contents) {
	FILE* file = fopen(path, "w");
	if (file) {
		fputs(contents, file);
		fclose(file);
	}
}
void getFile(char* path, char* buffer, size_t buffer_size) {
	FILE *file = fopen(path, "r");
	if (file) {
		fseek(file, 0L, SEEK_END);
		size_t size = ftell(file);
		if (size>buffer_size-1) size = buffer_size - 1;
		rewind(file);
		fread(buffer, sizeof(char), size, file);
		fclose(file);
		buffer[size] = '\0';
	}
}
char* allocFile(char* path) { // caller must free!
	char* contents = NULL;
	FILE *file = fopen(path, "r");
	if (file) {
		fseek(file, 0L, SEEK_END);
		size_t size = ftell(file);
		contents = calloc(size+1, sizeof(char));
		fseek(file, 0L, SEEK_SET);
		fread(contents, sizeof(char), size, file);
		fclose(file);
		contents[size] = '\0';
	}
	return contents;
}
int getInt(char* path) {
	int i = 0;
    if (path == NULL) {
        return i;
    }
    
	FILE *file = fopen(path, "r");
	if (file!=NULL) {
		int res = fscanf(file, "%i", &i);
		fclose(file);
        if(res != 1)
            i = 0; // failed to parse int
	}
	return i;
}
void putInt(char* path, int value) {
	char buffer[8];
	sprintf(buffer, "%d", value);
	putFile(path, buffer);
}

uint64_t getMicroseconds(void) {
    uint64_t ret;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ret = (uint64_t)tv.tv_sec * 1000000;
    ret += (uint64_t)tv.tv_usec;

    return ret;
}

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

int clamp(int x, int lower, int upper)
{
    return min(upper, max(x, lower));
}

double clampd(double x, double lower, double upper)
{
    return min(upper, max(x, lower));
}

char* findFileInDir(const char *directory, const char *filename) {
    char *filename_copy = strdup(filename);
    if (!filename_copy) {
        perror("strdup");
        return NULL;
    }

    // Strip extension from filename
    char *dot_pos = strrchr(filename_copy, '.');
    if (dot_pos) {
        *dot_pos = '\0';
    }

    DIR *dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        free(filename_copy);
        return NULL;
    }

    struct dirent *entry;
    char *full_path = NULL;

    // Track the best (shortest) match to avoid prefix collisions.
    // e.g., searching for "Advance Wars" should match "Advance Wars (USA).gba"
    // over "Advance Wars 2 - Black Hole Rising (USA).gba"
    char *best_match_name = NULL;
    size_t best_match_len = SIZE_MAX;

    while ((entry = readdir(dir)) != NULL) {
        // Strip extension from entry for comparison
        char *entry_base = strdup(entry->d_name);
        if (!entry_base) continue;

        char *entry_dot = strrchr(entry_base, '.');
        if (entry_dot) *entry_dot = '\0';

        if (strstr(entry_base, filename_copy) == entry_base) {
            // Prefer shorter matches (closer to exact match)
            size_t entry_len = strlen(entry_base);
            if (entry_len < best_match_len) {
                free(best_match_name);
                best_match_name = strdup(entry->d_name);
                best_match_len = entry_len;
            }
        }
        free(entry_base);
    }

    closedir(dir);

    if (best_match_name) {
        full_path = (char *)malloc(strlen(directory) + strlen(best_match_name) + 2);
        if (full_path) {
            snprintf(full_path, strlen(directory) + strlen(best_match_name) + 2, "%s/%s", directory, best_match_name);
        }
        free(best_match_name);
    }

    free(filename_copy);
    return full_path;
}
