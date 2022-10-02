/* garbage httpd, just to see if it works
 * easily DoSable (like the rest of the network stack), vulnerable to path traversal, etc */
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define eprintf(fmt, ...) fprintf(stderr, "httpd: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

static void handle(FILE *c) {
	char buf[2048];
	fgets(buf, sizeof buf, c);
	printf("%s", buf);

	if (memcmp(buf, "GET /", 5) != 0) {
		fprintf(c, "HTTP/1.1 400 Bad Request\r\n\r\n");
		return;
	}
	char *path = buf + 4;
	char *end = strchr(path, ' ');
	if (end) *end = '\0';

	handle_t h = _syscall_open(path, strlen(path), OPEN_READ);
	if (h < 0) {
		fprintf(c, "HTTP/1.1 404 Not Found\r\n\r\n");
		return;
	}
	FILE *f = fdopen(h, "r");
	if (!f) {
		fprintf(c, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
		return;
	}

	if (path[strlen(path) - 1] != '/') {
		/* regular file */
		fprintf(c, "HTTP/1.1 200 OK\r\n");
		fprintf(c, "\r\n");
		for (;;) {
			int len = fread(buf, 1, sizeof buf, f);
			if (len <= 0) break;
			fwrite(buf, 1, len, c);
		}
	} else {
		/* directory listing */
		fprintf(c, "HTTP/1.1 200 OK\r\n");
		fprintf(c, "Content-Type: text/html; charset=UTF-8\r\n");
		fprintf(c, "\r\n");
		fprintf(c, "<h1>directory listing for %s</h1><hr><ul><li><a href=..>..</a></li>", path);
		for (;;) {
			int len = fread(buf, 1, sizeof buf, f);
			if (len <= 0) break;
			// TODO directory library
			// based on find.c
			for (int pos = 0; pos < len; ) {
				if (buf[pos] == '\0') break;
				const char *end = memchr(buf + pos, 0, len - pos);
				if (!end) break;
				fprintf(c, "<li><a href=\"%s\">%s</a></li>", buf + pos, buf + pos);
				pos += end - (buf + pos) + 1;
			}
		}
	}
	fclose(f);
}

int main(int argc, char **argv) {
	const char *path = (argc > 1) ? argv[1] : "/net/listen/0.0.0.0/tcp/80";
	handle_t conn;
	for (;;) {
		conn = _syscall_open(path, strlen(path), OPEN_RW);
		if (conn < 0) {
			eprintf("open('%s') failed, %d", path, conn);
			return 1;
		}
		FILE *f = fdopen(conn, "a+");
		handle(f);
		fclose(f);
	}
}
