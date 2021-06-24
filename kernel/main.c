#include <kernel/tty.h>

void kmain()
{
	for (int i = 0; i < 400; i++)
		tty_write("words ", 6);
}
