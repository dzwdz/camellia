#include "elfload.h"
#include <unistd.h>

int main();

__attribute__((__weak__))
void _start(void) {
	elf_selfreloc();
	exit(main());
}
