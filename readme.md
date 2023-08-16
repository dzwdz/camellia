# camellia
an experimental, work-in-progress, microkernel based on some of my ideas for privilege separation.

## third party code used
* `src/libc/elf.h` from [adachristine](https://github.com/adachristine/sophia/tree/main/api/elf)
* `src/libc/vendor/getopt` from [skeeto](https://github.com/skeeto/getopt)
* `src/libc/vendor/dlmalloc` from [Doug Lea](https://gee.cs.oswego.edu/dl/html/malloc.html)
* `src/libk/include/bits/limits.h` based on [heatd](https://github.com/heatd/)'s code
* `src/kernel/arch/amd64/3rdparty/multiboot2.h` from the FSF

## build dependencies
```sh
# on debian
# TODO not yet verified on a clean install
apt-get install gcc git sparse # basics
apt-get install grub-pc-bin xorriso mtools # for the .iso
apt-get install g++ libgmp-dev lbmpfr-dev libmpc-dev # for the toolchain
apt-get install qemu-system-x86
```

## repo organization
```
src/
	bootstrap/	first userland program ran by the kernel; embeds the initrd
	cmd/	userland programs
	kernel/
		arch/amd64/
		sysroot/
			dummy sysroot to get gcc to behave
	libc/
	libk/	libc functions used by the kernel
cache/	download cache
```
