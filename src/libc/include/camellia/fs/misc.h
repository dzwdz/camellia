#pragma once
#include <stdbool.h>
#include <stdlib.h>

void forward_open(hid_t reqh, const char *path, long len, int flags);

void fs_passthru(const char *prefix);
void fs_whitelist(const char **list);
void fs_union(const char **list);

void fs_dirinject(const char *path);
void fs_dirinject2(const char *injects[]);

int mount_at(const char *path);

// TODO separate fs drivers and wrappers around syscalls

/** like _sys_fs_wait, but ensures *buf is a null terminated string on VFSOP_OPEN */
hid_t ufs_wait(char *buf, size_t len, struct ufs_request *req);

/** Mounts something and injects its path into the fs */
#define MOUNT_AT(path) for (; mount_at(path) == 0; exit(1))
