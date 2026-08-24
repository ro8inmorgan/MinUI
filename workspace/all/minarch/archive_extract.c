#include "archive_extract.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int has_traversal_component(const char *member) {
    const char *component = member;

    for (const char *p = member;; p++) {
        if (*p == '/' || *p == '\0') {
            size_t len = (size_t)(p - component);
            if ((len == 1 && component[0] == '.') ||
                (len == 2 && component[0] == '.' && component[1] == '.')) return 1;
            if (*p == '\0') return 0;
            component = p + 1;
        }
    }
}

int archive_member_basename(const char *member, char *name, size_t name_size) {
    const char *base;
    size_t len;

    if (!member || !member[0] || !name || !name_size || member[0] == '/' ||
        strchr(member, '\\') || has_traversal_component(member)) return -1;
    base = strrchr(member, '/');
    base = base ? base + 1 : member;
    len = strlen(base);
    if (!len || len >= name_size) return -1;
    memcpy(name, base, len + 1);
    return 0;
}

int archive_make_private_dir(char *path, size_t path_size) {
    static const char template[] = "/tmp/nextarch.XXXXXX";

    if (!path || path_size < sizeof(template)) return -1;
    memcpy(path, template, sizeof(template));
    if (!mkdtemp(path)) return -1;
    if (chmod(path, 448) != 0) {
        rmdir(path);
        return -1;
    }
    return 0;
}

int archive_open_output(const char *dir, const char *name, off_t expected_size, int *fd) {
    int dirfd;
    int output;
    struct stat st;

    if (!dir || !name || !fd || expected_size < 0) return -1;
    dirfd = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (dirfd < 0) return -1;
    output = openat(dirfd, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 384);
    if (output >= 0) {
        close(dirfd);
        *fd = output;
        return 0;
    }
    if (errno != EEXIST) {
        close(dirfd);
        return -1;
    }
    output = openat(dirfd, name, O_RDONLY | O_NOFOLLOW);
    close(dirfd);
    if (output < 0) return -1;
    if (fstat(output, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != expected_size) {
        close(output);
        return -1;
    }
    close(output);
    *fd = -1;
    return 1;
}

int archive_write_all(int fd, const void *buffer, size_t size) {
    const char *data = buffer;

    while (size) {
        ssize_t written = write(fd, data, size);
        if (written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (written == 0) return -1;
        data += written;
        size -= (size_t)written;
    }
    return 0;
}
