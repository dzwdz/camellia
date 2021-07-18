AS      = i686-elf-as
CC      = i686-elf-gcc
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
CFLAGS += -mgeneral-regs-only
CFLAGS += -Isrc/
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc
QFLAGS  = -no-reboot -d guest_errors,int,pcall,cpu_reset

define from_sources
  $(patsubst src/%.s,out/obj/%.s.o,$(shell find $(1) -type f -name '*.s')) \
  $(patsubst src/%.c,out/obj/%.c.o,$(shell find $(1) -type f -name '*.c'))
endef

out/boot.iso: out/fs/boot/kernel.bin out/fs/boot/grub/grub.cfg out/fs/boot/init
	grub-mkrescue -o $@ out/fs/

out/fs/boot/grub/grub.cfg: grub.cfg
	@mkdir -p $(@D)
	cp $< $@

out/fs/boot/init: src/test_module
	@mkdir -p $(@D)
	cp $< $@

out/fs/boot/kernel.bin: $(call from_sources, src/)
	@mkdir -p $(@D)
	$(CC) $(LFLAGS) -T linker.ld $^ -o $@
	grub-file --is-x86-multiboot $@

out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	$(AS) $^ -o $@

out/obj/%.c.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@


.PHONY: boot debug lint clean
boot: out/boot.iso
	qemu-system-i386 $< $(QFLAGS) -no-shutdown

debug: out/boot.iso
	qemu-system-i386 $< $(QFLAGS) -s -S &
	sleep 1
	gdb

lint:
	@tools/linter/main.rb

clean:
	rm -rv out/
