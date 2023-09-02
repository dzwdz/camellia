#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <camellia/fs/misc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <camellia.h>
#include <sys/param.h>

typedef struct {
	hid_t sock;
} Handle;

static char *socks_addr;

static void*
fs_open(char *path, int flags) {
	char *tokp;

	if (*path++ != '/') {
		return errno = ENOENT, NULL;
	}
	if (!OPEN_WRITEABLE(flags)) {
		return errno = EACCES, NULL;
	}

	const char *verb = strtok_r(path, "/", &tokp);
	if (!verb) {
		return errno = ENOENT, NULL;
	}
	if (strcmp(verb, "connect") != 0) {
		return errno = ENOSYS, NULL; // unimpl.
	}

	// TODO ip parsing in stdlib
	// uint32_t srcip;
	//if (ip_parse(strtok_r(NULL, "/", &tokp), &srcip) < 0 || srcip != 0) {
	//	return errno = ENOENT, NULL;
	//}
	strtok_r(NULL, "/", &tokp);

	char *dest = strtok_r(NULL, "/", &tokp);
	if (!dest) return errno = ENOENT, NULL;
	size_t destlen = strlen(dest);
	if (destlen >= 256) return errno = ENAMETOOLONG, NULL;
	// let's assume it's a domain

	char *proto = strtok_r(NULL, "/", &tokp);
	if (!proto) return errno = ENOENT, NULL;
	if (strcmp(proto, "tcp") != 0) {
		return errno = ENOSYS, NULL; // unimpl.
	}
	
	char *port_s = strtok_r(NULL, "/", &tokp);
	if (!port_s) return errno = ENOENT, NULL;
	uint16_t port = strtol(port_s, NULL, 0);

	Handle *h;
	h = calloc(1, sizeof(*h));
	if (!h) return errno = ENOMEM, NULL;

	h->sock = camellia_open(socks_addr, OPEN_READ | OPEN_WRITE);
	if (h->sock < 0) {
		goto err;
	}

	/* 0x05    version 5
	 * 0x01    one authentication method:
	 *   0x00  no auth */
	char buf[512];
	write(h->sock, "\x05\x01\x00", 3);

	errno = EGENERIC;
	ssize_t ret = read(h->sock, buf, 2);
	if (ret != 2) goto err; // yeah yeah i know
	if (memcmp(buf, "\x05\x00", 2) != 0) goto err;

	int p = 0;
	buf[p++] = 5; /* v5 */
	buf[p++] = 1; /* tcp connection */
	buf[p++] = 0; /* reserved */
	buf[p++] = 3; /* addr type, 3 = domain */
	buf[p++] = destlen; /* length, max 255 */
	memcpy(buf + p, dest, destlen);
	p += destlen;
	buf[p++] = port >> 8;
	buf[p++] = port;

	write(h->sock, buf, p);

	ret = read(h->sock, buf, 10);
	// just comparing to what tor replies, i have bigger issues than making this general
	if (ret != 10 || memcmp(buf, "\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10) != 0) {
		goto err;
	}

	return h;

err:
	close(h->sock);
	free(h);
	return NULL;
}

int main(int argc, char *argv[]) {
	const size_t buflen = 4096;
	char *buf = malloc(buflen);

	if (argc != 2) {
		fprintf(stderr, "usage: socksfs /net/connect/.../\n");
		return 1;
	}
	socks_addr = argv[1];

	struct ufs_request req;
	hid_t reqh;
	// TODO an fs_wait flag to only wait for OPENs, and then to only wait for a given handle
	// then i can use proper threads
	while ((reqh = ufs_wait(buf, buflen, &req))) {
		Handle *h = req.id;
		switch (req.op) {
			case VFSOP_OPEN: {
				void *ret = fs_open(buf, req.flags);
				if (ret) {
					_sys_fs_respond(reqh, ret, 0, 0);
				} else {
					_sys_fs_respond(reqh, NULL, -errno, 0);
				}
				break;
			}
			case VFSOP_READ: {
				if (_sys_fork(FORK_NOREAP, NULL) == 0) {
					int len = MIN(buflen, req.capacity);
					len = _sys_read(h->sock, buf, len, -1);
					_sys_fs_respond(reqh, buf, len, 0);
					exit(0);
				} else {
					close(reqh);
				}
				break;
			}
			case VFSOP_WRITE: {
				if (_sys_fork(FORK_NOREAP, NULL) == 0) {
					int len = _sys_write(h->sock, buf, req.len, -1, 0);
					_sys_fs_respond(reqh, NULL, len, 0);
					exit(0);
				} else {
					close(reqh);
				}
				break;
			}
			case VFSOP_CLOSE:
				close(h->sock);
				free(h);
				_sys_fs_respond(reqh, NULL, -1, 0);
				break;
			default:
				_sys_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
}
