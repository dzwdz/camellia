PATH   := $(shell pwd)/toolchain/bin/:$(PATH)

AS      = x86_64-elf-as
CC      = x86_64-elf-gcc
CHECK   = sparse
CFLAGS += -g -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Wold-style-definition -Werror=implicit-function-declaration -ftrack-macro-expansion=0
CFLAGS += -mgeneral-regs-only -Wno-address-of-packed-member
CFLAGS += -Isrc/
SPARSEFLAGS = -Wno-non-pointer-null
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc -Wl,-zmax-page-size=4096 -Wl,--no-warn-mismatch
QFLAGS  = -no-reboot
ifndef QEMU_DISPLAY
QFLAGS += -display none
endif

define from_sources
  $(patsubst src/%.s,out/obj/%.s.o,$(shell find $(1) -type f,l -name '*.s')) \
  $(patsubst src/%.c,out/obj/%.c.o,$(shell find $(1) -type f,l -name '*.c'))
endef


.PHONY: all boot debug lint check clean
all: out/boot.iso check

boot: all out/hdd
	qemu-system-x86_64 -drive file=out/boot.iso,format=raw,media=disk $(QFLAGS) -serial stdio

test: all
	@# pipes for the serial
	@mkfifo out/qemu.in out/qemu.out 2> /dev/null || true
	qemu-system-x86_64 -drive file=out/boot.iso,format=raw,media=disk $(QFLAGS) -serial pipe:out/qemu &
	@# for some reason the first sent character doesn't go through to the shell
	@# the empty echo takes care of that, so the next echos will work just fine
	@echo > out/qemu.in
	echo run_tests > out/qemu.in
	echo exit > out/qemu.in
	@echo
	@cat out/qemu.out

debug: all
	qemu-system-x86_64 -cdrom out/boot.iso $(QFLAGS) -serial stdio -s -S &
	@sleep 1
	gdb

lint:
	@tools/linter/main.rb

check: $(shell find src/kernel/ -type f -name *.c)
	@echo $^ | xargs -n 1 sparse $(CFLAGS) $(SPARSEFLAGS)

clean:
	rm -rv out/


out/boot.iso: out/fs/boot/kernel.bin out/fs/boot/grub/grub.cfg out/fs/boot/init
	@grub-mkrescue -o $@ out/fs/ > /dev/null 2>&1

out/fs/boot/kernel.bin: src/kernel/linker.ld $(call from_sources, src/kernel/) $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@
	grub-file --is-x86-multiboot $@

out/bootstrap: src/user_bootstrap/linker.ld $(call from_sources, src/user_bootstrap/) $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@

initrd/test.elf: out/test.elf
	@# dummy

out/test.elf: src/usertestelf.ld out/obj/usertestelf.c.o out/obj/user/lib/syscall.s.o $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -Wl,-pie -Wl,-no-dynamic-linker -T $^ -o $@

# TODO automatically resolve symlinks
out/initrd.tar: $(shell find initrd/) out/test.elf
	cd initrd; tar chf ../$@ *

out/fs/boot/init: out/bootstrap out/initrd.tar
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

out/obj/usertestelf.c.o: src/usertestelf.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -fPIE -c $^ -o $@

out/obj/shared/%.c.o: src/shared/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -fPIC -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.c.o: src/kernel/arch/amd64/32/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -m32 -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.s.o: src/kernel/arch/amd64/32/%.s
	@mkdir -p $(@D)
	@$(CC) -m32 -c $^ -o $@

src/user/lib/syscall.c: src/user/lib/syscall.c.awk src/shared/syscalls.h
	awk -f $^ > $@
