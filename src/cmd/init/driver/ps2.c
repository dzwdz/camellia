#include "driver.h"
#include <assert.h>
#include <camellia.h>
#include <camellia/compat.h>
#include <camellia/syscalls.h>
#include <err.h>
#include <errno.h>
#include <shared/ring.h>
#include <stdbool.h>
#include <stdlib.h>
#include <thread.h>


static const char keymap_lower[] = {
	'\0', '\0', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\r', '\0', 'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  '\0', '\'', '`',  '\0', '\\', 'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',  '\0', '*',  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
};

static const char keymap_upper[] = {
	'\0', '\0', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
	'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\r', '\0', 'A',  'S',
	'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X',  'C',  'V',
	'B',  'N',  'M',  '<',  '>',  '?',  '\0', '*',  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	'2',  '3',  '0',  '.',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
	'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
};

static volatile uint8_t backlog_buf[16];
static volatile ring_t backlog = {(void*)backlog_buf, sizeof backlog_buf, 0, 0};

static bool keys[0x80] = {0};

static void parse_scancode(uint8_t s) {
	bool ctrl  = keys[0x1D];
	bool shift = keys[0x2A] || keys[0x36];
	bool down = !(s & 0x80);
	char c;
	s &= 0x7f;
	keys[s] = down;

	c = shift ? keymap_upper[s] : keymap_lower[s];
	if (ctrl && keymap_upper[s] >= 'A' && keymap_upper[s] <= 'Z')
		c = keymap_upper[s] - 'A' + 1;
	if (down && c) ring_put1b((void*)&backlog, c);
}


#define QSIZE 16
static hid_t queue[QSIZE] = {0};

static void enqueue(hid_t h) {
	for (int i = 0; i < QSIZE; i++) {
		if (queue[i] == 0) {
			queue[i] = h;
			return;
		}
	}
	_sys_fs_respond(h, NULL, -EAGAIN, 0);
}

static void fulfill(void) {
	int ret;
	char c;
	bool queued = false;
	for (int i = 0; i < QSIZE; i++) {
		if (queue[i] != 0) {
			queued = true;
			break;
		}
	}
	if (!queued) return;

	// only reading a single char at a time because that's easier
	ret = ring_get((void*)&backlog, &c, 1);
	if (ret == 0) return;
	for (int i = 0; i < QSIZE; i++) {
		if (queue[i] != 0) {
			_sys_fs_respond(queue[i], &c, 1, 0);
			queue[i] = 0;
			break;
		}
	}
}

static void kb_thread(void *unused) {
	static char buf[512];
	int fd;
	(void)unused;

	fd = camellia_open("/dev/ps2/kb", OPEN_READ);
	if (fd < 0) err(1, "open");

	while (true) {
		int ret = _sys_read(fd, buf, sizeof buf, -1);
		if (ret < 0) break;
		for (int i = 0; i < ret; i++) {
			parse_scancode(buf[i]);
		}
		fulfill();
	}
}

static void fs_thread(void *unused) {
	static char buf[512];
	(void)unused;
	for (;;) {
		struct ufs_request res;
		hid_t reqh = _sys_fs_wait(buf, sizeof buf, &res);
		if (reqh < 0) return;

		switch (res.op) {
		case VFSOP_OPEN:
			if (res.len == 0) {
				_sys_fs_respond(reqh, NULL, 1, 0);
			} else {
				_sys_fs_respond(reqh, NULL, -ENOENT, 0);
			}
			break;

		case VFSOP_READ:
			enqueue(reqh);
			fulfill();
			break;

		default:
			_sys_fs_respond(reqh, NULL, -1, 0);
			break;
		}
	}
}

void ps2_drv(void) {
	thread_create(0, kb_thread, NULL);
	thread_create(0, fs_thread, NULL);
	_sys_await();
	exit(0);
}
