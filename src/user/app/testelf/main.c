#include <camellia/syscalls.h>
#include <stdio.h>
#include <user/lib/elf.h>
#include <user/lib/elfload.h>
#include <unistd.h>

const char *str = "Hello!\n", *str2 = "World.\n";

__attribute__((visibility("hidden")))
extern char _image_base[];

int main(void) {
	elf_selfreloc();
	printf("loaded at %x\n", &_image_base);
	printf(str);
	printf(str2);
	exit(0);
}
