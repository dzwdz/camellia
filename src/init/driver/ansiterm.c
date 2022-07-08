#include <init/driver/driver.h>
#include <shared/syscalls.h>
#include <stdbool.h>

struct vga_cell {
	unsigned char c;
	unsigned char style;
} __attribute__((__packed__));
static struct vga_cell vga[80 * 25];

static handle_t vga_fd;

static struct {int x, y;} cursor = {0};
static bool dirty = false;
static bool pendingFlush = false;

static void flush(void) {
	size_t off = 0;
	/* we have to do multiple write() calls if we're behind a shitty passthrough fs
	 * i don't like this either */
	while (off < sizeof(vga))
		off += _syscall_write(vga_fd, (void*)vga + off, sizeof(vga) - off, off);
	dirty = false;
	pendingFlush = false;
}

static void scroll(void) {
	for (size_t i = 0; i < 80 * 24; i++)
		vga[i] = vga[i + 80];
	for (size_t i = 80 * 24; i < 80 * 25; i++)
		vga[i].c = ' ';
	cursor.y--;
	pendingFlush = true;
}

static void in_char(char c) {
	switch (c) {
		case '\n':
			cursor.x = 0;
			cursor.y++;
			pendingFlush = true;
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
	dirty = true;
}

void ansiterm_drv(void) {
	vga_fd = _syscall_open("/kdev/vga", 9, 0);
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

	for (int i = 0; i < 80 * 25; i++)
		vga[i].style = 0x70;
	flush();

	static char buf[512];
	struct fs_wait_response res;
	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				if (res.flags & OPEN_CREATE) {
					_syscall_fs_respond(NULL, -1, 0);
					break;
				}
				// TODO check path
				_syscall_fs_respond(NULL, 0, 0);
				break;

			case VFSOP_WRITE:
				for (size_t i = 0; i < res.len; i++)
					in_char(buf[i]);
				/* if (pendingFlush) */ flush();
				_syscall_fs_respond(NULL, res.len, 0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}

	_syscall_exit(1);
}
