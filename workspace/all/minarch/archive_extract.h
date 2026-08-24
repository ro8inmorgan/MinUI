#ifndef ARCHIVE_EXTRACT_H
#define ARCHIVE_EXTRACT_H

#include <stddef.h>
#include <sys/types.h>

int archive_member_basename(const char *member, char *name, size_t name_size);
int archive_make_private_dir(char *path, size_t path_size);
int archive_open_output(const char *dir, const char *name, off_t expected_size, int *fd);
int archive_write_all(int fd, const void *buffer, size_t size);

#endif
