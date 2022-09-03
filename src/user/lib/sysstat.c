#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
