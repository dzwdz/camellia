#include <errno.h>
#include <stdio.h>

void perror(const char *s) {
	if (s) fprintf(stderr, "%s: ", s);
	fprintf(stderr, "errno %d\n", errno);
}

int puts(const char *s) {
	return printf("%s\n", s);
}

int getchar(void) {
	return fgetc(stdin);
}

int putchar(int c) {
	return fputc(c, stdout);
}

off_t lseek(int fd, off_t off, int whence) {
	(void)fd; (void)off; (void)whence;
	errno = ENOSYS;
	return -1;
}
