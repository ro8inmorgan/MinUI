#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "archive_extract.h"
#include "path_helpers.h"
#include "utils.h"

int main(void) {
    char emu[64];
    char basename[64];
    char directory[] = "/tmp/nextui-archive-XXXXXX";
    char *name;
    char *quoted;
    char *valid_argv[] = { "minarch", "core", "rom", NULL };
    char *long_argv[] = { "minarch", "12345678", "rom", NULL };
    int fd;

    quoted = shell_quote_alloc("a'b");
    assert(quoted && strcmp(quoted, "'a'\"'\"'b'") == 0);
    free(quoted);

    name = nextui_path_remove_extension("a");
    assert(name && strcmp(name, "a") == 0);
    free(name);
    name = removeExtension(".");
    assert(name && strcmp(name, ".") == 0);
    free(name);
    name = removeExtension("file.");
    assert(name && strcmp(name, "file.") == 0);
    free(name);
    name = removeExtension("a.b");
    assert(name && strcmp(name, "a.b") == 0);
    free(name);

    assert(path_get_emu_name(emu, sizeof(emu), "/mnt/SDCARD/Roms/Game Boy (GB", "/mnt/SDCARD/Roms") == 0);
    assert(strcmp(emu, "Game Boy (GB") == 0);
    assert(path_get_emu_name(emu, sizeof(emu), "/mnt/SDCARD/Roms/Game Boy (GB)/rom.gb", "/mnt/SDCARD/Roms") == 0);
    assert(strcmp(emu, "GB") == 0);
    getEmuName("/var/tmp/nextui/sdcard/Roms/Game Boy (GB", emu);
    assert(strcmp(emu, "Game Boy (GB") == 0);
    getEmuName("/var/tmp/nextui/sdcard/Roms/Game Boy (GB)/rom.gb", emu);
    assert(strcmp(emu, "GB") == 0);
    assert(path_copy(emu, 4, "long") == -1 && emu[0] == '\0');

    char folder[8], display[8], long_path[8192], long_name[8192];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 6] = '/';
    memcpy(long_path + sizeof(long_path) - 5, "file", 5);
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    assert(folderPathSafe("alpha/file", folder, sizeof(folder)) == 0 && strcmp(folder, "alpha") == 0);
    assert(folderPathSafe(long_path, folder, sizeof(folder)) == -1 && folder[0] == '\0');
    assert(getDisplayNameSafe("folder/game.zip", display, sizeof(display)) == 0 && strcmp(display, "game") == 0);
    assert(getDisplayNameSafe(long_name, display, sizeof(display)) == -1 && display[0] == '\0');

    assert(!path_arguments_fit(2, valid_argv, 8));
    assert(path_arguments_fit(3, valid_argv, 8));
    assert(!path_arguments_fit(3, long_argv, 8));

    assert(archive_member_basename("folder/game.gb", basename, sizeof(basename)) == 0);
    assert(strcmp(basename, "game.gb") == 0);
    assert(archive_member_basename("../game.gb", basename, sizeof(basename)) == -1);
    assert(archive_member_basename("folder/../../game.gb", basename, sizeof(basename)) == -1);
    assert(archive_member_basename("", basename, sizeof(basename)) == -1);
    char long_member[513];
    char long_basename[512];
    memset(long_member, 'a', sizeof(long_member) - 1);
    long_member[sizeof(long_member) - 1] = '\0';
    assert(archive_member_basename(long_member, long_basename, sizeof(long_basename)) == -1);

    assert(mkdtemp(directory));
    char from[128], to[128], relative[4];
    assert(snprintf(from, sizeof(from), "%s/from", directory) > 0);
    assert(snprintf(to, sizeof(to), "%s/to", directory) > 0);
    assert(mkdir(from, 448) == 0 && mkdir(to, 448) == 0);
    assert(pathRelativeToSafe(relative, sizeof(relative), from, to) == -1);
    assert(rmdir(from) == 0 && rmdir(to) == 0);

    char from_component[128], to_component[128], to_file[128], relative_component[128];
    assert(snprintf(from_component, sizeof(from_component), "%s/bc", directory) > 0);
    assert(snprintf(to_component, sizeof(to_component), "%s/bcd", directory) > 0);
    assert(snprintf(to_file, sizeof(to_file), "%s/rom", to_component) > 0);
    assert(mkdir(from_component, 0700) == 0 && mkdir(to_component, 0700) == 0);
    assert((fd = open(to_file, O_WRONLY | O_CREAT, 0600)) >= 0 && close(fd) == 0);
    assert(pathRelativeToSafe(relative_component, sizeof(relative_component), from_component, to_file) == 0);
    assert(strcmp(relative_component, "../bcd/rom") == 0);
    assert(unlink(to_file) == 0 && rmdir(from_component) == 0 && rmdir(to_component) == 0);
    char link_path[128];
    assert(snprintf(link_path, sizeof(link_path), "%s/game.gb", directory) > 0);
    assert(symlink("/tmp/not-a-rom", link_path) == 0);
    assert(archive_open_output(directory, "game.gb", 3, &fd) == -1);
    assert(unlink(link_path) == 0);
    assert(archive_open_output(directory, "game.gb", 3, &fd) == 0);
    assert(archive_write_all(fd, "rom", 3) == 0);
    assert(close(fd) == 0);
    assert(rmdir(directory) == 0 || (unlink(link_path) == 0 && rmdir(directory) == 0));
    return 0;
}
