# camellia
an experimental, work-in-progress, microkernel based on some of my ideas for privilege separation.

## third party code used
* `src/user/lib/elf.h` from [adachristine](https://github.com/adachristine/sophia/tree/main/api/elf)
* `src/user/lib/vendor/getopt` from [skeeto](https://github.com/skeeto/getopt)
* `src/user/lib/vendor/dlmalloc` from [Doug Lea](https://gee.cs.oswego.edu/dl/html/malloc.html)
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
