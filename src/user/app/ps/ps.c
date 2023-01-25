#include <_proc.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* returns a pointer that can be set to NUL to undo the strcat */
static char *
strtcat(char *dst, const char *src)
{
	char *s = dst + strlen(dst);
	strcpy(s, src);
	return s;
}

static void
do_proc(char *path)
{
	const int bufl = 4096;
	char *buf = malloc(bufl);
	FILE *f;

	{ /* read the psdata into buf */
		char *s = strtcat(path, "mem");
		f = fopen(path, "r");
		*s = '\0';
		if (!f) errx(1, "couldn't open '%s'", path);
		fseek(f, (long)_libc_psdata, SEEK_SET);
		if (fread(buf, 1, 128, f) <= 0) {
			strcpy(buf, "(no psdata)");
		}
		buf[128] = '\0';
		fclose(f);
	}

	printf("%20s %s\n", path, buf);

	f = fopen(path, "r");
	if (!f) errx(1, "couldn't open '%s'", path);

	// TODO library for iterating over directories
	for (;;) {
		int len = fread(buf, 1, bufl, f);
		if (len <= 0) break;
		for (int pos = 0; pos < len; ) {
			const char *end = memchr(buf + pos, 0, len - pos);
			if (!end) {
				errx(1, "unimplemented: buffer overflow");
			}
			size_t entryl = end - (buf + pos) + 1;
			if (isdigit(buf[pos])) {
				/* yup, no overflow check */
				char *s = strtcat(path, buf + pos);
				do_proc(path);
				*s = '\0';
			}
			pos += entryl;
		}
	}

	free(buf);
	fclose(f);
}

int
main(void)
{
	char *buf = malloc(4096);
	strcpy(buf, "/proc/");
	do_proc(buf);
	return 0;
}
