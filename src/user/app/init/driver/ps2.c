#include "driver.h"
#include <camellia/syscalls.h>
#include <shared/container/ring.h>
#include <stdbool.h>
#include <stdlib.h>
#include <camellia/compat.h>


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

static hid_t fd;

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

static void main_loop(void) {
	static char buf[512];
	struct ufs_request res;
	int ret;
	while (!c0_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				c0_fs_respond(NULL, 1, 0);
				break;

			case VFSOP_READ:
				while (ring_used((void*)&backlog) == 0) {
					/* read raw input until we have something to output */
					int len = _sys_read(fd, buf, sizeof buf, 0);
					if (len == 0) break;
					for (int i = 0; i < len; i++)
						parse_scancode(buf[i]);
				}
				ret = ring_get((void*)&backlog, buf, res.capacity);
				c0_fs_respond(buf, ret, 0);
				break;

			default:
				c0_fs_respond(NULL, -1, 0);
				break;
		}
	}
}

void ps2_drv(void) {
	fd = _sys_open("/kdev/ps2/kb", 12, 0);
	if (fd < 0) exit(1);

	main_loop();
	exit(0);
}
