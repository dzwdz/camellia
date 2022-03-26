#include <kernel/arch/i386/tty/keyboard.h>

static volatile bool keyboard_pressed = false;
static volatile char keyboard_char;

static bool keys[0x80] = {0};

void keyboard_recv(char s) {
	char c = 0, d = 0;
	bool down = !(s & 0x80);
	keys[s & 0x7f] = down;

	switch (s & 0x7f) {
		case 0x01:	/* esc */				break;
		case 0x02:	c = '1';	d = '!';	break;
		case 0x03:	c = '2';	d = '@';	break;
		case 0x04:	c = '3';	d = '#';	break;
		case 0x05:	c = '4';	d = '$';	break;
		case 0x06:	c = '5';	d = '%';	break;
		case 0x07:	c = '6';	d = '^';	break;
		case 0x08:	c = '7';	d = '&';	break;
		case 0x09:	c = '8';	d = '*';	break;
		case 0x0A:	c = '9';	d = '(';	break;
		case 0x0B:	c = '0';	d = ')';	break;
		case 0x0C:	c = '-';	d = '_';	break;
		case 0x0D:	c = '=';	d = '+';	break;
		case 0x0E:	c = '\b';	d = '\b';	break;
		case 0x0F:	c = '\t';	d = '\t';	break;
		case 0x10:	c = 'q';	d = 'Q';	break;
		case 0x11:	c = 'w';	d = 'W';	break;
		case 0x12:	c = 'e';	d = 'E';	break;
		case 0x13:	c = 'r';	d = 'R';	break;
		case 0x14:	c = 't';	d = 'T';	break;
		case 0x15:	c = 'y';	d = 'Y';	break;
		case 0x16:	c = 'u';	d = 'U';	break;
		case 0x17:	c = 'i';	d = 'I';	break;
		case 0x18:	c = 'o';	d = 'O';	break;
		case 0x19:	c = 'p';	d = 'P';	break;
		case 0x1A:	c = '[';	d = '{';	break;
		case 0x1B:	c = ']';	d = '}';	break;
		case 0x1C:	c = '\r';	d = '\r';	break;
		case 0x1D:	/*lctrl*/				break;
		case 0x1E:	c = 'a';	d = 'A';	break;
		case 0x1F:	c = 's';	d = 'S';	break;
		case 0x20:	c = 'd';	d = 'D';	break;
		case 0x21:	c = 'f';	d = 'F';	break;
		case 0x22:	c = 'g';	d = 'G';	break;
		case 0x23:	c = 'h';	d = 'H';	break;
		case 0x24:	c = 'j';	d = 'J';	break;
		case 0x25:	c = 'k';	d = 'K';	break;
		case 0x26:	c = 'l';	d = 'L';	break;
		case 0x27:	c = ';';	d = ':';	break;
		case 0x28:	c = '\'';	d = '"';	break;
		case 0x29:	c = '`';	d = '~';	break;
		case 0x2A:	/*lshift */				break;
		case 0x2B:	c = '\\';	d = '|';	break;
		case 0x2C:	c = 'z';	d = 'Z';	break;
		case 0x2D:	c = 'x';	d = 'X';	break;
		case 0x2E:	c = 'c';	d = 'C';	break;
		case 0x2F:	c = 'v';	d = 'V';	break;
		case 0x30:	c = 'b';	d = 'B';	break;
		case 0x31:	c = 'n';	d = 'N';	break;
		case 0x32:	c = 'm';	d = 'M';	break;
		case 0x33:	c = ',';	d = '<';	break;
		case 0x34:	c = '.';	d = '>';	break;
		case 0x35:	c = '/';	d = '?';	break;
		case 0x36:	/*rshift*/				break;
		case 0x38:	/*lalt*/				break;
		case 0x39:	c = ' ';	d = ' ';	break;
		case 0x3A:	/*caps*/	break;
		case 0x3B:	/* F1 */	break;
		case 0x3C:	/* F2 */	break;
		case 0x3D:	/* F3 */	break;
		case 0x3E:	/* F4 */	break;
		case 0x3F:	/* F5 */	break;
		case 0x40:	/* F6 */	break;
		case 0x41:	/* F7 */	break;
		case 0x42:	/* F8 */	break;
		case 0x43:	/* F9 */	break;
		case 0x44:	/* F10 */	break;
		case 0x45:	/*numlock*/	break;
		case 0x46:	/*scroll^*/	break;
		case 0x57:	/* F11 */	break;
		case 0x58:	/* F12 */	break;

		/* keypad */
		case 0x37:	c = '*';	d = '*';	break;
		case 0x47:	c = '7';	d = '7';	break;
		case 0x48:	c = '8';	d = '8';	break;
		case 0x49:	c = '9';	d = '9';	break;
		case 0x4A:	c = '-';	d = '-';	break;
		case 0x4B:	c = '4';	d = '4';	break;
		case 0x4C:	c = '5';	d = '5';	break;
		case 0x4D:	c = '6';	d = '6';	break;
		case 0x4E:	c = '+';	d = '+';	break;
		case 0x4F:	c = '1';	d = '1';	break;
		case 0x50:	c = '2';	d = '2';	break;
		case 0x51:	c = '3';	d = '3';	break;
		case 0x52:	c = '0';	d = '0';	break;
		case 0x53:	c = '.';	d = '.';	break;
	}

	if (c && down) {
		bool shift = keys[0x2A] || keys[0x36];
		keyboard_pressed = true;
		keyboard_char = shift ? d : c;
	}
}

bool keyboard_poll_read(char *c) {
	if (!keyboard_pressed) return false;
	keyboard_pressed = false;
	*c = keyboard_char;
	return true;
}


