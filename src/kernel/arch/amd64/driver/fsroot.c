#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/driver.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static long handle(VfsReq *req) {
	// TODO document directory read format
	// TODO don't hardcode
	static const char base[] = "kdev/";
	static const char dir[] =
		"com1\0"
		"ps2/\0"
		"ata/\0"
		"eth\0"
		"video/";
	const char *id;
	int len;

	if (!req->caller) return -1;
	if (req->type != VFSOP_OPEN) {
		/* otherwise, uninitialized, to cause compiler warnings if used */
		/* this operates under the assumption that base and dir never change
		 * but even if they do, that will just result in a crash. */
		id = (void *__force)req->id;
		if (id == base) {
			len = sizeof base;
		} else if (id == dir) {
			len = sizeof dir;
		} else {
			kprintf("%p %p %p\n", base, dir, id);
			assert(false);
		}
	}

	switch (req->type) {
		case VFSOP_OPEN:
			if (reqpathcmp(req, "/")) return (long)base;
			if (reqpathcmp(req, "/kdev/")) return (long)dir;
			return -ENOENT;

		case VFSOP_READ:
			return req_readcopy(req, id, len);
		// TODO getsize for the other kernel provided directories
		case VFSOP_GETSIZE:
			return len;
		default:
			return -ENOSYS;
	}
}

static void accept(VfsReq *req) {
	vfsreq_finish_short(req, handle(req));
}

void vfs_root_init(void) { vfs_root_register("", accept); }
