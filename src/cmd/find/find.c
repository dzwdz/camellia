#include <camellia/path.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void recurse(char *path) {
	DIR *d = opendir(path);
	if (!d) {
		warn("couldn't open %s", path);
		return;
	}
	for (;;) {
		struct dirent *dent;
		errno = 0;
		dent = readdir(d);
		if (!dent) {
			if (errno) {
				warn("when reading %s", path);
			}
			break;
		}
		printf("%s%s\n", path, dent->d_name);
		/* if the string ends with '/' */
		if (strchr(dent->d_name, '\0')[-1] == '/') {
			// TODO no overflow check
			char *pend = strchr(path, '\0');
			strcpy(pend, dent->d_name);
			recurse(path);
			*pend = '\0';
		}
	}
	closedir(d);
}

void find(const char *path) {
	// TODO bound checking
	// TODO or just implement asprintf()
	char *buf = malloc(PATH_MAX);
	memcpy(buf, path, strlen(path)+1);
	recurse(buf);
	free(buf);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		find("/");
	} else {
		for (int i = 1; i < argc; i++)
			find(argv[i]);
	}
	return 0;
}
