#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "atomic_settings.h"

int main(int argc, char **argv) {
    struct stat st;
    const char first[] = "old";
    const char second[] = "new settings";

    assert(argc == 2);
    assert(atomic_settings_write(argv[1], first, sizeof(first)) == 0);
    assert(atomic_settings_write(argv[1], second, sizeof(second)) == 0);
    assert(stat(argv[1], &st) == 0);
    assert((st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == (S_IRUSR | S_IWUSR));

    FILE *file = fopen(argv[1], "r");
    char actual[sizeof(second)] = {0};
    assert(file != NULL);
    assert(fread(actual, 1, sizeof(actual), file) == sizeof(actual));
    assert(fclose(file) == 0);
    assert(memcmp(actual, second, sizeof(second)) == 0);
    return 0;
}
