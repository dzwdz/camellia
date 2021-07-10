AS      = i686-elf-as
CC      = i686-elf-gcc
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS += -mgeneral-regs-only
CFLAGS += -Isrc/
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc
QFLAGS  = -no-reboot -d guest_errors,int,pcall,cpu_reset

OBJ  = $(patsubst src/%.s,out/obj/%.s.o,$(shell find src/ -type f -name '*.s'))
OBJ += $(patsubst src/%.c,out/obj/%.c.o,$(shell find src/ -type f -name '*.c'))


.PHONY: boot debug clean
boot: out/fs/boot/kernel.bin
	qemu-system-i386 -kernel $< $(QFLAGS) -no-shutdown

debug: kernel.bin
	qemu-system-i386 -kernel $< $(QFLAGS) -s -S &
	sleep 1
	gdb

clean:
	rm -rv out/


out/boot.iso: out/fs/boot/kernel.bin out/fs/boot/grub/grub.cfg
	grub-mkrescue -o $@ out/fs/

out/fs/boot/grub/grub.cfg: grub.cfg
	@mkdir -p $(@D)
	cp $< $@

out/fs/boot/kernel.bin: $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(LFLAGS) -T linker.ld $^ -o $@
	grub-file --is-x86-multiboot $@

out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	$(AS) $^ -o $@

out/obj/%.c.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@
