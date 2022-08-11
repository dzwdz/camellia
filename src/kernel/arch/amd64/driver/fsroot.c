#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/panic.h>
#include <kernel/util.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static int handle(struct vfs_request *req) {
	// TODO document directory read format
	// TODO don't hardcode
	const char dir[] =
		"com1\0"
		"ps2/\0"
		"ata/\0"
		"video/";
	if (!req->caller) return -1;
	switch (req->type) {
		case VFSOP_OPEN:
			return req->input.len == 1 ? 0 : -1;

		case VFSOP_READ:
			return req_readcopy(req, dir, sizeof dir);

		// TODO getsize for the other kernel provided directories
		case VFSOP_GETSIZE: return sizeof dir;
		default: return -ENOSYS;
	}
}

static void accept(struct vfs_request *req) {
	vfsreq_finish_short(req, handle(req));
}

static struct vfs_backend backend = BACKEND_KERN(accept);
void vfs_root_init(void) { vfs_mount_root_register("", &backend); }
