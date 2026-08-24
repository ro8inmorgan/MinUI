#ifndef ATOMIC_SETTINGS_H
#define ATOMIC_SETTINGS_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static inline int atomic_settings_write(const char *path, const void *data, size_t size) {
    char temporary[512];
    char directory[512];
    const char *slash;
    int fd;
    int dir_fd;
    size_t written = 0;

    if (snprintf(temporary, sizeof(temporary), "%s.tmp.%ld", path, (long)getpid()) >= (int)sizeof(temporary)) return -1;
    fd = open(temporary, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd < 0) return -1;
    if (fchmod(fd, S_IRUSR | S_IWUSR) != 0) goto failed;
    while (written < size) {
        ssize_t result = write(fd, (const char *)data + written, size - written);
        if (result < 0) {
            if (errno == EINTR) continue;
            goto failed;
        }
        if (result == 0) goto failed;
        written += (size_t)result;
    }
    if (fdatasync(fd) != 0) goto failed;
    if (close(fd) != 0) goto failed_unlinked;
    if (rename(temporary, path) != 0) goto failed_unlinked;
    slash = strrchr(path, '/');
    if (!slash || (size_t)(slash - path) >= sizeof(directory)) return -1;
    memcpy(directory, path, (size_t)(slash - path));
    directory[slash - path] = '\0';
    dir_fd = open(directory[0] ? directory : "/", O_RDONLY);
    if (dir_fd < 0) return -1;
    if (fsync(dir_fd) != 0) {
        close(dir_fd);
        return -1;
    }
    return close(dir_fd);

failed:
    close(fd);
failed_unlinked:
    unlink(temporary);
    return -1;
}

static inline int atomic_settings_save_due(struct timespec *last_save, int force) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return force;
    if (!force && (now.tv_sec == last_save->tv_sec) && (now.tv_nsec - last_save->tv_nsec) < 250000000L) return 0;
    *last_save = now;
    return 1;
}

#endif
