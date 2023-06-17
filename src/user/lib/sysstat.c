#include <camellia.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int fstat(int fd, struct stat *sb) {
	(void)fd;
	memset(sb, 0, sizeof sb);
	// TODO save info if it was a dir
	sb->st_mode = 0777 | S_IFREG;
	return 0;
}

int stat(const char *restrict path, struct stat *restrict sb) {
	int fd, ret;
	fd = camellia_open(path, OPEN_READ);
	if (fd < 0) {
		return -1;
	}
	ret = fstat(fd, sb);
	close(fd);
	return ret;
}

int lstat(const char *restrict path, struct stat *restrict sb) {
	// TODO assumes no symlink support
	return stat(path, sb);
}

int mkdir(const char *path, mode_t mode) {
	// TODO error when directory already exits
	// TODO fopen-like wrapper that calls open() with a processed path
	(void)mode;
	size_t plen = strlen(path);
	char *tmp = NULL;
	/* ensure trailing slash */
	if (plen >= 1 && path[plen - 1] != '/') {
		tmp = malloc(plen + 2);
		memcpy(tmp, path, plen);
		tmp[plen] = '/';
		tmp[plen + 1] = '\0';
		path = tmp;
	}
	FILE *f = fopen(path, "a"); /* sets errno */
	if (f) fclose(f);
	free(tmp);
	return f == NULL ? -1 : 0;
}
