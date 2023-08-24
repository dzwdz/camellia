#pragma once
#include <bits/panic.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h> // struct timespec
#include <errno.h> // only for ENOSYS

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;

	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;

#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};

#define S_IFMT  0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000

#define S_IRUSR 0x400

/* inode(7) */
#define S_ISREG(m) ((m & S_IFMT) == S_IFREG)
#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) ((m & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) ((m & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) ((m & S_IFMT) == S_IFIFO)
#define S_ISLNK(m) ((m & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) ((m & S_IFMT) == S_IFSOCK)

int fstat(int fd, struct stat *sb);
int stat(const char *restrict path, struct stat *restrict sb);
int lstat(const char *restrict path, struct stat *restrict sb);
int mkdir(const char *path, mode_t mode);

static inline mode_t umask(mode_t mask) {
	(void)mask;
	return 0;
}

static inline int chmod(const char *path, mode_t mode) {
	(void)path; (void)mode;
	errno = ENOSYS;
	return -1;
}

static inline int fchmod(int fd, mode_t mode) {
	(void)fd; (void)mode;
	errno = ENOSYS;
	return -1;
}

static inline int mknod(const char *path, mode_t mode, dev_t dev) {
	(void)path; (void)mode; (void)dev;
	errno = ENOSYS;
	return -1;
}
