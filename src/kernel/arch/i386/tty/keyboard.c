#include <kernel/arch/i386/tty/keyboard.h>
#include <kernel/arch/i386/tty/ps2_keymap.h>

static volatile bool keyboard_pressed = false;
static volatile char keyboard_char;

static bool keys[0x80] = {0};

void keyboard_recv(uint8_t s) {
	bool shift = keys[0x2A] || keys[0x36];
	bool down = !(s & 0x80);
	s &= 0x7f;
	keys[s] = down;

	char c = shift ? keymap_upper[s] : keymap_lower[s];

	if (down && c) {
		keyboard_char = c;
		keyboard_pressed = true;
	}
}

bool keyboard_poll_read(char *c) {
	if (!keyboard_pressed) return false;
	keyboard_pressed = false;
	*c = keyboard_char;
	return true;
}


