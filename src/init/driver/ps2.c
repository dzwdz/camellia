#include <init/driver/driver.h>
#include <shared/syscalls.h>
#include <stdbool.h>


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


#define BACKLOG_CAPACITY 16
static char backlog[BACKLOG_CAPACITY] = {};
static size_t backlog_size = 0;

static handle_t fd;

static bool keys[0x80] = {0};

static void parse_scancode(uint8_t s) {
	bool shift = keys[0x2A] || keys[0x36];
	bool down = !(s & 0x80);
	s &= 0x7f;
	keys[s] = down;

	char c = shift ? keymap_upper[s] : keymap_lower[s];

	if (backlog_size >= BACKLOG_CAPACITY) return;
	if (down && c)
		backlog[backlog_size++] = c;
}

static void main_loop(void) {
	static char buf[512];
	struct fs_wait_response res;
	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				_syscall_fs_respond(NULL, 1);
				break;

			case VFSOP_READ:
				while (backlog_size == 0) {
					/* read raw input until we have something to output */
					int len = _syscall_read(fd, buf, sizeof buf, 0);
					if (len == 0) break;
					for (int i = 0; i < len; i++)
						parse_scancode(buf[i]);
				}
				if (res.capacity > backlog_size)
					res.capacity = backlog_size;
				// TODO don't discard chars past what was read
				backlog_size = 0;
				_syscall_fs_respond(backlog, res.capacity);
				break;

			default:
				_syscall_fs_respond(NULL, -1);
				break;
		}
	}
}

void ps2_drv(void) {
	fd = _syscall_open("/ps2", 4);
	if (fd < 0) _syscall_exit(1);

	main_loop();
	_syscall_exit(0);
}
