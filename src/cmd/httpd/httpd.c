/* garbage httpd, just to see if it works
 * easily DoSable (like the rest of the network stack), vulnerable to path traversal, etc */
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

	hid_t h = _sys_open(path, strlen(path), OPEN_READ);
	if (h < 0) {
		fprintf(c, "HTTP/1.1 404 Not Found\r\n\r\n");
		return;
	}

	if (path[strlen(path) - 1] != '/') {
		FILE *f = fdopen(h, "r");
		if (!f) {
			fprintf(c, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
			return;
		}

		/* regular file */
		fprintf(c, "HTTP/1.1 200 OK\r\n\r\n");
		for (;;) {
			int len = fread(buf, 1, sizeof buf, f);
			if (len <= 0) break;
			fwrite(buf, 1, len, c);
		}

		fclose(f);
	} else {
		/* directory listing */
		DIR *dir = opendir_f(fdopen(h, "r"));
		if (!dir) {
			fprintf(c, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
			return;
		}

		fprintf(c,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=UTF-8\r\n"
			"\r\n"
			"<h1>directory listing for %s</h1><hr>"
			"<ul><li><a href=..>..</a></li>",
			path
		);

		struct dirent *d;
		while ((d = readdir(dir))) {
			fprintf(c, "<li><a href=\"%s\">%s</a></li>", d->d_name, d->d_name);
		}
		closedir(dir);
	}
}

int main(int argc, char **argv) {
	const char *path = (argc > 1) ? argv[1] : "/net/listen/0.0.0.0/tcp/80";
	hid_t conn;
	for (;;) {
		conn = _sys_open(path, strlen(path), OPEN_RW);
		if (conn < 0)
			errx(1, "open('%s') failed, errno %d", path, -conn);
		FILE *f = fdopen(conn, "a+");
		handle(f);
		fclose(f);
	}
}
