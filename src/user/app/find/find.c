#include <camellia/path.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, "find: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

void recurse(char *path) {
	FILE *f = fopen(path, "r");
	const int buf_len = 4096;
	char *buf;

	if (!f) {
		eprintf("couldn't open %s", path);
		return;
	}

	buf = malloc(buf_len);
	for (;;) {
		int len = fread(buf, 1, buf_len, f);
		int pos = 0;
		if (len <= 0) break;
		while (pos < len) {
			if (buf[pos] == '\0') {
				eprintf("%s has an empty entry, bailing", path);
				break;
			}
			const char *end = memchr(buf + pos, 0, len - pos);
			if (!end) {
				eprintf("buf overflowed, unimplemented"); // TODO
				break;
			}
			printf("%s%s\n", path, buf + pos);

			size_t entrylen = end - (buf + pos) + 1;
			if (end[-1] == '/') {
				// append onto end of the current path
				// TODO no overflow check
				char *pend = path + strlen(path);
				memcpy(pend, buf + pos, entrylen);
				recurse(path);
				*pend = '\0';
			}
			pos += entrylen;
		}
	}
	fclose(f);
	free(buf);
}

void find(const char *path) {
	// TODO bound checking
	char *buf = malloc(PATH_MAX);
	memcpy(buf, path, strlen(path)+1);
	recurse(buf);
	free(buf);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		recurse("/");
	} else {
		for (int i = 1; i < argc; i++)
			recurse(argv[i]);
	}
	return 0;
}
