#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

int remove(const char *path) {
	return unlink(path);
}

// TODO! VFSOP_MOVE
int rename(const char *old, const char *new) {
	(void)old; (void)new;
	errno = ENOSYS;
	return -1;
}

// TODO tmpnam
char *tmpnam(char *s) {
	static char buf[L_tmpnam];
	if (!s) s = buf;
	strcpy(s, "/tmp/tmpnam");
	return s;
}
