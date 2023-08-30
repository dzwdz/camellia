#include <_proc.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
usage(void)
{
	fprintf(stderr, "usage: ps [path]\n");
	exit(1);
}

int
main(int argc, const char *argv[])
{
	const char *path = argc >= 2 ? argv[1] : "/proc/";
	if (argc > 2) usage();

	char *procbuf = malloc(4096);
	DIR *dir = opendir(path);
	if (!dir) {
		err(1, "couldn't open %s", path);
	}

	struct dirent *de;
	while ((de = readdir(dir))) {
		const char *name = de->d_name;
		if (isdigit(name[0])) {
			FILE *g;
			sprintf(procbuf, "%s%smem", path, name);
			g = fopen(procbuf, "r");
			if (!g) {
				warn("couldn't open \"%s\"", procbuf);
				strcpy(procbuf, "(can't peek)");
			} else {
				fseek(g, (long)&_psdata_loc->desc, SEEK_SET);
				if (fread(procbuf, 1, 128, g) <= 0) {
					strcpy(procbuf, "(no psdata)");
				}
				procbuf[128] = '\0';
				fclose(g);
			}
			*strchr(name, '/') = '\0';
			printf("%s\t%s\n", name, procbuf);
		}
	}

	free(procbuf);
	closedir(dir);
	return 0;
}
