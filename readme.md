# camellia
an experimental, work-in-progress, microkernel based on some of my ideas for privilege separation.

this README is shit, but it's still better than the old one

## the ideas
### everything is a file, but really now
All resources are accessed through the filesystem, similarly to Plan 9. `/kdev/ata/0`, `/net/0.0.0.0/listen/tcp/80`, etc.
This makes it easier to reason about the privileges of a process, as they're all managed in the same way.
On Linux, `namespaces(7)` lists 8 different process namespaces - 8 separate ways to isolate each part of a process. It's easy to let something slip through.

Compare that to `whitelist /bin/httpd:ro /var/www/:ro /net/0.0.0.0/listen/tcp/80 -- httpd`. It's immediately obvious what httpd can and can't do.

Internally, that works somewhat like Plan 9 namespaces. `whitelist` hijacks all `open()` calls made by its children, and decides if they can pass through.
This stacks easily. Broadly speaking, processes have full control over their childrens' resource accesses. Filesystem drivers and such use that too.
This also makes them easy to sandbox: `mount /mnt/ whitelist /bin/fatfs /kdev/ata/0 -- fatfs /kdev/ata/0`.


## list of stolen stuff
* `src/user/lib/elf.h` from [adachristine](https://github.com/adachristine/sophia/tree/main/api/elf)
* `src/user/lib/vendor/getopt` from [skeeto](https://github.com/skeeto/getopt)
* `src/user/lib/vendor/dlmalloc` from [Doug Lea](https://gee.cs.oswego.edu/dl/html/malloc.html)
* `src/kernel/arch/amd64/3rdparty/multiboot2.h` from the FSF
