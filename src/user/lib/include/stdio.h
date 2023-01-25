#pragma once
#include <bits/file.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#define EOF (-1)
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

#define _IONBF 0
#define _IOFBF 0
#define _IOLBF 1

/* size of file buffers. not that we have any */
#define BUFSIZ 1024

/* stop fread() from trying to fill the entire buffer before returning
 * i.e. it will call _sys_read() exactly once */
#define FEXT_NOFILL 1

int printf(const char *restrict fmt, ...);
int fprintf(FILE *restrict f, const char *restrict fmt, ...);

int sprintf(char *restrict s, const char *restrict fmt, ...);

int vprintf(const char *restrict fmt, va_list ap);
int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);

int _klogf(const char *fmt, ...); // for kernel debugging only


extern FILE *const stdin, *const stdout, *const stderr;

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *);
FILE *fdopen(int fd, const char *mode);
FILE *file_clone(const FILE *, const char *mode);
FILE *popen(const char *cmd, const char *mode);
int pclose(FILE *f);
FILE *tmpfile(void);

int fextflags(FILE *, int extflags);
int setvbuf(FILE *restrict f, char *restrict buf, int type, size_t size);
int fclose(FILE *);
int fflush(FILE *f);

size_t fread(void *restrict ptr, size_t size, size_t nitems, FILE *restrict);
size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict);
int fputs(const char *s, FILE *f);
char *fgets(char *buf, int size, FILE *f);
int fgetc(FILE *f);
int getc(FILE *f);
int fputc(int c, FILE *f);
int putc(int c, FILE *f);
int ungetc(int c, FILE *f);

int fseek(FILE *f, long offset, int whence);
int fseeko(FILE *f, off_t offset, int whence);
long ftell(FILE *f);
off_t ftello(FILE *f);

int feof(FILE *);
int ferror(FILE *);
void clearerr(FILE *f);

void perror(const char *s);
int puts(const char *s);
int getchar(void);
int putchar(int c);

off_t lseek(int fd, off_t off, int whence);

int remove(const char *path);
int rename(const char *old, const char *new);

#define L_tmpnam (5 + 16 + 1)
char *tmpnam(char *s);

int sscanf(const char *restrict s, const char *restrict format, ...);
