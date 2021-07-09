AS      = i686-elf-as
CC      = i686-elf-gcc
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS += -mgeneral-regs-only
CFLAGS += -I.
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc
QFLAGS  = -no-reboot -d guest_errors,int,pcall,cpu_reset

OBJ  = $(patsubst %.s,%.o,$(wildcard platform/*.s))
OBJ += $(patsubst %.c,%.o,$(wildcard kernel/*.c))


.PHONY: boot debug clean
boot: kernel.bin
	qemu-system-i386 -kernel kernel.bin $(QFLAGS) -no-shutdown

debug: kernel.bin
	qemu-system-i386 -kernel kernel.bin $(QFLAGS) -s -S &
	sleep 1
	gdb

clean:
	rm -vf kernel.bin
	rm -vf **/*.o

boot.iso: kernel.bin grub.cfg
	mkdir iso_tmp
	mkdir -p iso_tmp/boot/grub
	cp kernel.bin iso_tmp/boot
	cp grub.cfg   iso_tmp/boot/grub
	grub-mkrescue -o $@ iso_tmp
	rm -rv iso_tmp

kernel.bin: $(OBJ)
	$(CC) $(LFLAGS) -T linker.ld $^ -o $@
	grub-file --is-x86-multiboot $@

platform/%.o: platform/%.s
	$(AS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@
