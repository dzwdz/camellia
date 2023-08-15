PATH   := $(shell pwd)/toolchain/prefix/bin/:$(PATH)

AR      = x86_64-camellia-ar
AS      = x86_64-camellia-as
CC      = x86_64-camellia-gcc

CFLAGS += -g -std=gnu99 -O2 -ftrack-macro-expansion=0
CFLAGS += -Wall -Wextra -Wold-style-definition -Werror=implicit-function-declaration
CFLAGS += -Wno-address-of-packed-member -Werror=incompatible-pointer-types

KERNEL_CFLAGS  = $(CFLAGS) -ffreestanding -mno-sse -mgeneral-regs-only \
	--sysroot=src/kernel/sysroot/ -Isrc/ -Isrc/libk/include/ -fno-pie
LIBC_CFLAGS    = $(CFLAGS) -ffreestanding -Isrc/
USER_CFLAGS    = $(CFLAGS)

SPARSEFLAGS = -$(KERNEL_CFLAGS) -Wno-non-pointer-null

PORTS =

define from_sources
  $(patsubst src/%,out/obj/%.o,$(shell find $(1) -type f,l -name '*.[csS]'))
endef


.PHONY: all portdeps check clean ports
all: portdeps out/boot.iso check out/fs.e2
portdeps: out/libc.a out/libm.a src/libc/include/__errno.h

check: $(shell find src/kernel/ -type f -name *.c)
	@echo $^ | xargs -n 1 sparse $(SPARSEFLAGS)

clean:
	rm -rf out/


out/boot.iso: out/fs/boot/kernel out/fs/boot/grub/grub.cfg out/fs/boot/init
	@grub-mkrescue -o $@ out/fs/ > /dev/null 2>&1

out/fs/boot/kernel: src/kernel/arch/amd64/linker.ld \
                    $(call from_sources, src/kernel/) \
                    $(call from_sources, src/libk/)
	@mkdir -p $(@D)
	@$(CC) \
		-nostdlib \
		-Wl,-zmax-page-size=4096 -Wl,--no-warn-mismatch -Wl,-no-pie \
		-T $^ -o $@
	@grub-file --is-x86-multiboot2 $@ || echo "$@ has an invalid multiboot2 header"
	@grub-file --is-x86-multiboot2 $@ || rm $@; test -e $@

out/libc.a: $(call from_sources, src/libc/) $(call from_sources, src/libk/)
	@mkdir -p $(@D)
	@$(AR) rcs $@ $^

out/libm.a:
	@mkdir -p $(@D)
	@$(AR) rcs $@ $^

out/bootstrap: out/bootstrap.elf
	@objcopy -O binary $^ $@

out/bootstrap.elf: src/bootstrap/linker.ld $(call from_sources, src/bootstrap/) out/libc.a
	@mkdir -p $(@D)
	@$(CC) -nostdlib -Wl,-no-pie -T $^ -o $@

out/fs/boot/init: out/bootstrap out/initrd.tar
	@mkdir -p $(@D)
	@cat $^ > $@

out/fs/boot/grub/grub.cfg: src/kernel/arch/amd64/grub.cfg
	@mkdir -p $(@D)
	@cp $< $@

out/fs.e2:
	@mkfs.ext2 $@ 1024 > /dev/null

define userbin_template =
out/initrd/bin/amd64/$(1): $(call from_sources, src/cmd/$(1)) out/libc.a
	@mkdir -p $$(@D)
	@$(CC) $$^ -o $$@
endef
USERBINS := $(shell ls src/cmd)
$(foreach bin,$(USERBINS),$(eval $(call userbin_template,$(bin))))

# don't build the example implementation from libext2
out/obj/cmd/ext2fs/ext2/example.c.o:
	@touch $@

# portdeps is phony, so ports/% is automatically "phony" too
ports: $(patsubst %,ports/%,$(PORTS))
ports/%: portdeps
	+$@/port install

out/initrd/%: sysroot/%
	@mkdir -p $(@D)
	@cp $< $@

out/initrd.tar: $(patsubst sysroot/%,out/initrd/%,$(shell find sysroot/ -type f)) \
                $(patsubst %,out/initrd/bin/amd64/%,$(USERBINS)) \
                $(shell find out/initrd/) \
				ports
	@cd out/initrd; tar chf ../initrd.tar *


out/obj/%.s.o: src/%.s
	@mkdir -p $(@D)
	@$(AS) $^ -o $@

out/obj/%.S.o: src/%.S
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $^ -o $@

out/obj/libk/%.c.o: src/libk/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -fPIC -c $^ -o $@

out/obj/kernel/%.c.o: src/kernel/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -fPIC -c $^ -o $@

out/obj/%.c.o: src/%.c
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -fPIC -c $^ -o $@

out/obj/libc/%.c.o: src/libc/%.c
	@mkdir -p $(@D)
	@$(CC) $(LIBC_CFLAGS) -fPIC -c $^ -o $@

out/obj/libc/vendor/%.c.o: src/libc/vendor/%.c
	@mkdir -p $(@D)
	@$(CC) $(LIBC_CFLAGS) -fPIC -c $^ -o $@ \
		-DLACKS_TIME_H -DLACKS_FCNTL_H -DLACKS_SYS_PARAM_H \
		-DMAP_ANONYMOUS -DHAVE_MORECORE=0 -DNO_MALLOC_H \
		-Wno-expansion-to-defined -Wno-old-style-definition

out/obj/bootstrap/%.c.o: src/bootstrap/%.c
	@mkdir -p $(@D)
	@$(CC) $(USER_CFLAGS) -fno-pic -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.c.o: src/kernel/arch/amd64/32/%.c
	@mkdir -p $(@D)
	@$(CC) $(KERNEL_CFLAGS) -m32 -c $^ -o $@

out/obj/kernel/arch/amd64/32/%.s.o: src/kernel/arch/amd64/32/%.s
	@mkdir -p $(@D)
	@$(CC) -m32 -c $^ -o $@

src/libc/include/__errno.h: src/libc/include/__errno.h.awk src/libk/include/camellia/errno.h
	@awk -f $^ > $@

src/libc/syscall.c: src/libc/syscall.c.awk src/libk/include/camellia/syscalls.h
	@awk -f $^ > $@
