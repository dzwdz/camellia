#include <stdint.h>

int main() {
	// change the colors of VGA text
	// doesn't require a lot of code, but still shows that it's working
	uint8_t *vga = (void*) 0xB8000;
	for (int i = 0; i < 80 * 25; i++)
		vga[(i << 1) + 1] = 0x4e;
	for (;;);
}