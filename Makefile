PATH   := $(shell pwd)/toolchain/bin/:$(PATH)

AS      = i686-elf-as
CC      = i686-elf-gcc
CHECK   = sparse
CFLAGS += -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Wold-style-definition -Werror=implicit-function-declaration -ftrack-macro-expansion=0
CFLAGS += -mgeneral-regs-only
CFLAGS += -Isrc/
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc
QFLAGS  = -no-reboot
ifndef QEMU_DISPLAY
QFLAGS += -display none
endif

define from_sources
  $(patsubst src/%.s,out/obj/%.s.o,$(shell find $(1) -type f -name '*.s')) \
  $(patsubst src/%.c,out/obj/%.c.o,$(shell find $(1) -type f -name '*.c'))
endef


.PHONY: all boot debug lint check clean
all: out/boot.iso check

boot: all out/hdd
	qemu-system-i386 -drive file=out/boot.iso,format=raw,media=disk $(QFLAGS) -serial stdio

test: all
	@# pipes for the serial
	@mkfifo out/qemu.in out/qemu.out 2> /dev/null || true
	qemu-system-i386 -drive file=out/boot.iso,format=raw,media=disk $(QFLAGS) -serial pipe:out/qemu &
	@# for some reason the first sent character doesn't go through to the shell
	@# the empty echo takes care of that, so the next echos will work just fine
	@echo > out/qemu.in
	echo run_tests > out/qemu.in
	echo exit > out/qemu.in
	@echo
	@cat out/qemu.out

debug: all
	qemu-system-i386 -cdrom out/boot.iso $(QFLAGS) -serial stdio -s -S &
	@sleep 1
	gdb

lint:
	@tools/linter/main.rb

check: $(shell find src/kernel/ -type f -name *.c)
	@echo $^ | xargs -n 1 sparse $(CFLAGS)

clean:
	rm -rv out/


out/boot.iso: out/fs/boot/kernel.bin out/fs/boot/grub/grub.cfg out/fs/boot/init
	@grub-mkrescue -o $@ out/fs/ > /dev/null 2>&1

out/fs/boot/kernel.bin: src/kernel/linker.ld $(call from_sources, src/kernel/) $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@
	grub-file --is-x86-multiboot $@

out/raw_init: src/init/linker.ld $(call from_sources, src/init/) $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@

out/initrd.tar: $(shell find initrd/)
	cd initrd; tar cf ../$@ *

out/fs/boot/init: out/raw_init out/initrd.tar
	@mkdir -p $(@D)
	@cat $^ > $@

out/fs/boot/grub/grub.cfg: grub.cfg
	@mkdir -p $(@D)
	@cp $< $@

out/hdd:
	@dd if=/dev/zero of=$@ bs=512 count=70000


out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	@$(AS) $^ -o $@

out/obj/%.c.o: src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

src/init/syscalls.c: tools/syscall_wrappers.awk src/shared/syscalls.h
	awk -f $^ > $@
