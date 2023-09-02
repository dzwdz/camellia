#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

DIR *opendir(const char *name) {
	return opendir_f(fopen(name, "r"));
}

DIR *opendir_f(FILE *fp) {
	if (!fp) return NULL;

	DIR *dir;
	dir = calloc(1, sizeof *dir);
	if (!dir) {
		fclose(fp);
		return NULL;
	}
	dir->fp = fp;
	return dir;
}

int closedir(DIR *dir) {
	fclose(dir->fp);
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir) {
	int i = 0;
	char *buf = dir->dent.d_name;
	for (;;) {
		int c = fgetc(dir->fp);
		if (c == EOF) {
			if (i == 0) return NULL;
			else break;
		}
		if (c == '\0') {
			break;
		}
		if (i == sizeof(dir->dent.d_name)-1) {
			/* overflow */
			for (;;) {
				c = fgetc(dir->fp);
				if (c == EOF || c == '\0') break;
			}
			return errno = ENAMETOOLONG, NULL;
		}
		buf[i++] = c;
	}
	buf[i] = '\0';
	return &dir->dent;
}
