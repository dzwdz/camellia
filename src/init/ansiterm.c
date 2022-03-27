#include <shared/syscalls.h>

struct vga_cell {
	unsigned char c;
	unsigned char style;
} __attribute__((__packed__));
static struct vga_cell vga[80 * 25];

static handle_t tty_fd;
static handle_t vga_fd;

static struct {int x, y;} cursor = {0};


static void flush(void) {
	_syscall_write(vga_fd, vga, sizeof vga, 0);
}

static void scroll(void) {
	for (size_t i = 0; i < 80 * 24; i++)
		vga[i] = vga[i + 80];
	for (size_t i = 80 * 24; i < 80 * 25; i++)
		vga[i].c = ' ';
	cursor.y--;
}

static void in_char(char c) {
	switch (c) {
		case '\n':
			cursor.x = 0;
			cursor.y++;
			break;
		case '\b':
			if (--cursor.x < 0) cursor.x = 0;
			break;
		default:
			vga[cursor.y * 80 + cursor.x++].c = c;
	}

	if (cursor.x >= 80) {
		cursor.x = 0;
		cursor.y++;
	}
	while (cursor.y >= 25) scroll();
}

void ansiterm_drv(void) {
	tty_fd = _syscall_open("/tty", 4);
	_syscall_write(tty_fd, "ansiterm...\n", 12, 0);

	vga_fd = _syscall_open("/vga", 4);
	_syscall_read(vga_fd, vga, sizeof vga, 0);

	// find first empty line
	for (cursor.y = 0; cursor.y < 25; cursor.y++) {
		for (cursor.x = 0; cursor.x < 80; cursor.x++) {
			char c = vga[cursor.y * 80 + cursor.x].c;
			if (c != ' ' && c != '\0') break;
		}
		if (cursor.x == 80) break;
	}
	cursor.x = 0;

	static char buf[512];
	struct fs_wait_response res;
	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				// TODO check path
				_syscall_fs_respond(NULL, 1);
				break;

			case VFSOP_WRITE:
				_syscall_write(tty_fd, buf, res.len, 0);
				for (int i = 0; i < res.len; i++)
					in_char(buf[i]);
				flush();
				_syscall_fs_respond(NULL, res.len);
				break;

			case VFSOP_READ:
				if (res.capacity > sizeof buf)
					res.capacity = sizeof buf;
				size_t len = _syscall_read(tty_fd, buf, res.capacity, 0);
				_syscall_fs_respond(buf, len);
				break;

			default:
				_syscall_fs_respond(NULL, -1);
				break;
		}
	}

	_syscall_exit(1);
}
