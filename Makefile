PATH   := $(shell pwd)/toolchain/bin/:$(PATH)

AS      = x86_64-elf-as
CC      = x86_64-elf-gcc
CHECK   = sparse

CFLAGS += -g -std=gnu99 -ffreestanding -O2 -ftrack-macro-expansion=0
CFLAGS += -Wall -Wextra -Wold-style-definition -Werror=implicit-function-declaration
CFLAGS += -mgeneral-regs-only -Wno-address-of-packed-member
CFLAGS += -Isrc/ -Isrc/shared/include/

KERNEL_CFLAGS  = $(CFLAGS)
USER_CFLAGS    = $(CFLAGS) -Isrc/user/lib/include/

SPARSEFLAGS = -Wno-non-pointer-null
LFLAGS  = -ffreestanding -O2 -nostdlib -lgcc -Wl,-zmax-page-size=4096 -Wl,--no-warn-mismatch
QFLAGS  = -no-reboot
ifndef NO_KVM
QFLAGS += -enable-kvm
endif
ifndef QEMU_DISPLAY
QFLAGS += -display none
endif


define from_sources
  $(patsubst src/%,out/obj/%.o,$(shell find $(1) -type f,l -name '*.[cs]'))
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
	echo tests > out/qemu.in
	echo halt > out/qemu.in
	@echo
	@cat out/qemu.out

debug: all
	qemu-system-x86_64 -cdrom out/boot.iso $(QFLAGS) -serial stdio -s -S &
	@sleep 1
	gdb

check: $(shell find src/kernel/ -type f -name *.c)
	@echo $^ | xargs -n 1 sparse $(CFLAGS) $(SPARSEFLAGS)
	@tools/linter/main.rb

clean:
	rm -rf out/


out/boot.iso: out/fs/boot/kernel out/fs/boot/grub/grub.cfg out/fs/boot/init
	@grub-mkrescue -o $@ out/fs/ > /dev/null 2>&1

out/fs/boot/kernel: src/kernel/linker.ld \
                    $(call from_sources, src/kernel/) \
                    $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -T $^ -o $@
	@grub-file --is-x86-multiboot $@ || echo "$@ has an invalid multiboot header"
	@grub-file --is-x86-multiboot $@ || rm $@; test -e $@

out/bootstrap: src/user/bootstrap/linker.ld \
               $(call from_sources, src/user/bootstrap/) \
               $(call from_sources, src/user/lib/) \
               $(call from_sources, src/shared/)
	@mkdir -p $(@D)
	@$(CC) $(LFLAGS) -Wl,-Map=% -T $^ -o $@

out/fs/boot/init: out/bootstrap out/initrd.tar
	@mkdir -p $(@D)
	@cat $^ > $@

out/fs/boot/grub/grub.cfg: grub.cfg
	@mkdir -p $(@D)
	@cp $< $@

out/hdd:
	@dd if=/dev/zero of=$@ bs=512 count=70000


define userbin_template =
out/initrd/bin/amd64/$(1): src/user/linker.ld \
                           $(call from_sources, src/user/app/$(1)/) \
                           $(call from_sources, src/user/lib/) \
                           $(call from_sources, src/shared/)
	@mkdir -p $$(@D)
	@$(CC) $(LFLAGS) -Wl,-pie -Wl,-no-dynamic-linker -T $$^ -o $$@
endef
USERBINS := $(shell ls src/user/app)
$(foreach bin,$(USERBINS),$(eval $(call userbin_template,$(bin))))

out/initrd/%: initrd/%
	@mkdir -p $(@D)
	@cp $< $@

out/initrd/font.psf: /usr/share/kbd/consolefonts/default8x16.psfu.gz
	@gunzip $< -c > $@

out/initrd.tar: $(patsubst %,out/%,$(shell find initrd/ -type f)) \
                $(patsubst %,out/initrd/bin/amd64/%,$(USERBINS)) \
                out/initrd/font.psf
	@cd out/initrd; tar chf ../initrd.tar *


out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	@$(AS) $^ -o $@

out/obj/shared/%.c.o: src/shared/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -fPIC -c $^ -o $@

out/obj/kernel/%.c.o: src/kernel/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -fPIC -c $^ -o $@

out/obj/user/%.c.o: src/user/%.c
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -fPIC -c $^ -o $@

out/obj/user/lib/vendor/%.c.o: src/user/lib/vendor/%.c
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -fPIC -c $^ -o $@ \
		-DLACKS_TIME_H -DLACKS_FCNTL_H -DLACKS_SYS_PARAM_H \
		-DMAP_ANONYMOUS -DHAVE_MORECORE=0 -DNO_MALLOC_H \
		-Wno-expansion-to-defined -Wno-old-style-definition

out/obj/user/bootstrap/%.c.o: src/user/bootstrap/%.c
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.c.o: src/kernel/arch/amd64/32/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -m32 -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.s.o: src/kernel/arch/amd64/32/%.s
	@mkdir -p $(@D)
	@$(CC) -m32 -c $^ -o $@

src/user/lib/syscall.c: src/user/lib/syscall.c.awk src/shared/include/camellia/syscalls.h
	@awk -f $^ > $@
