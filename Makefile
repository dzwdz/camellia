AS      = i686-elf-as
CC      = i686-elf-gcc
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS += -I.
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc

OBJ  = platform/boot.o
OBJ += $(patsubst %.c,%.o,$(wildcard kernel/*.c))


.PHONY: boot clean
boot: kernel.bin
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -vf kernel.bin
	rm -vf **/*.o


kernel.bin: $(OBJ)
	$(CC) $(LFLAGS) -T linker.ld $^ -o $@
	grub-file --is-x86-multiboot $@

platform/boot.o: platform/boot.s
	$(AS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@
