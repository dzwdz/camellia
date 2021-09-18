PATH   := $(shell pwd)/toolchain/bin/:$(PATH)

AS      = i686-elf-as
CC      = i686-elf-gcc
CHECK   = sparse
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Wold-style-definition
CFLAGS += -mgeneral-regs-only
CFLAGS += -Isrc/
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc
QFLAGS  = -no-reboot -serial stdio -display none

define from_sources
  $(patsubst src/%.s,out/obj/%.s.o,$(shell find $(1) -type f -name '*.s')) \
  $(patsubst src/%.c,out/obj/%.c.o,$(shell find $(1) -type f -name '*.c'))
endef


.PHONY: all boot debug lint check clean
all: out/boot.iso check

boot: all
	qemu-system-i386 -cdrom out/boot.iso $(QFLAGS) -no-shutdown

debug: all
	qemu-system-i386 -cdrom out/boot.iso $(QFLAGS) -s -S &
	@sleep 1
	gdb

lint:
	@tools/linter/main.rb

check: $(shell find src/kernel/ -type f -name *.c)
	@echo $^ | xargs -n 1 sparse $(CFLAGS) -Wno-non-pointer-null -Wno-decl

clean:
	rm -rv out/


out/boot.iso: out/fs/boot/kernel.bin out/fs/boot/grub/grub.cfg out/fs/boot/init
	@grub-mkrescue -o $@ out/fs/ > /dev/null 2>&1

out/fs/boot/kernel.bin: src/kernel/linker.ld $(call from_sources, src/kernel/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@
	grub-file --is-x86-multiboot $@

out/raw_init: src/init/linker.ld $(call from_sources, src/init/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@

out/initrd.tar: $(shell find initrd/)
	tar cf $@ initrd/

out/fs/boot/init: out/raw_init out/initrd.tar
	@mkdir -p $(@D)
	@cat $^ > $@

out/fs/boot/grub/grub.cfg: grub.cfg
	@mkdir -p $(@D)
	@cp $< $@


out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	@$(AS) $^ -o $@

out/obj/%.c.o: src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@
