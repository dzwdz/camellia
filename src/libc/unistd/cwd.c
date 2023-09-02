#include <camellia.h>
#include <camellia/path.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *cwd = NULL, *cwd2 = NULL;
static size_t cwdcapacity = 0;

static const char *getrealcwd(void) {
	if (cwd) return cwd;
	if (__initialcwd) return __initialcwd;
	return "/";
}

int chdir(const char *path) {
	hid_t h;
	char *tmp;
	size_t len = absolutepath(NULL, path, 0) + 1; /* +1 for the trailing slash */
	if (cwdcapacity < len) {
		cwdcapacity = len;
		if (cwd) {
			cwd = realloc(cwd, len);
			cwd2 = realloc(cwd2, len);
		} else {
			size_t initlen = strlen(__initialcwd) + 1;
			if (len < initlen)
				len = initlen;
			cwd = malloc(initlen);
			cwd2 = malloc(initlen);
			memcpy(cwd, __initialcwd, initlen);
		}
	}
	absolutepath(cwd2, path, cwdcapacity);
	len = strlen(cwd2);
	if (cwd2[len - 1] != '/') {
		cwd2[len] = '/';
		cwd2[len + 1] = '\0';
	}

	/* check if exists */
	h = camellia_open(cwd2, OPEN_READ);
	if (h < 0) return errno = ENOENT, -1;
	close(h);

	tmp  = cwd;
	cwd  = cwd2;
	cwd2 = tmp;
	return 0;
}

char *getcwd(char *buf, size_t capacity) {
	const char *realcwd = getrealcwd();
	size_t len = strlen(realcwd) + 1;
	if (capacity < len) {
		errno = capacity == 0 ? EINVAL : ERANGE;
		return NULL;
	}
	memcpy(buf, realcwd, len);
	return buf;
}

size_t absolutepath(char *out, const char *in, size_t size) {
	const char *realcwd = getrealcwd();
	size_t len, pos = 0;
	if (!in) return strlen(realcwd) + 1;

	if (!(in[0] == '/')) {
		len = strlen(realcwd);
		if (pos + len <= size && out != realcwd)
			memcpy(out + pos, realcwd, len);
		pos += len;

		if (realcwd[len - 1] != '/') {
			if (pos + 1 <= size) out[pos] = '/';
			pos++;
		}
	}

	len = strlen(in);
	if (pos + len <= size)
		memcpy(out + pos, in, len);
	pos += len;

	if (pos <= size) {
		pos = path_simplify(out, out, pos);
		if (pos == 0) return 0;
	}

	if (pos + 1 <= size) out[pos] = '\0';
	pos++;

	return pos;
}
