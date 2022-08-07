#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/driver/fsroot.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <shared/mem.h>
#include <stdbool.h>

enum {
	HANDLE_ROOT,
	HANDLE_ATA_ROOT,
	HANDLE_ATA,
	_SKIP = HANDLE_ATA + 4,
};

static bool exacteq(struct vfs_request *req, const char *str) {
	size_t len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

static int handle(struct vfs_request *req) {
	assert(req->caller);
	int id = (int)(long __force)req->id;
	switch (req->type) {
		case VFSOP_OPEN:
			if (exacteq(req, "/"))		return HANDLE_ROOT;
			if (exacteq(req, "/ata/"))	return HANDLE_ATA_ROOT;
			if (exacteq(req, "/ata/0"))
				return ata_available(0) ? HANDLE_ATA+0 : -1;
			if (exacteq(req, "/ata/1"))
				return ata_available(1) ? HANDLE_ATA+1 : -1;
			if (exacteq(req, "/ata/2"))
				return ata_available(2) ? HANDLE_ATA+2 : -1;
			if (exacteq(req, "/ata/3"))
				return ata_available(3) ? HANDLE_ATA+3 : -1;

			return -1;

		case VFSOP_READ:
			switch (id) {
				case HANDLE_ROOT: {
					// TODO document directory read format
					// TODO don't hardcode
					const char src[] =
						"com1\0"
						"ps2/\0"
						"ata/\0"
						"video/";
					return req_readcopy(req, src, sizeof src);
				}
				case HANDLE_ATA_ROOT: {
					char list[8] = {};
					size_t len = 0;
					for (int i = 0; i < 4; i++) {
						if (ata_available(i)) {
							list[len] = '0' + i;
							len += 2;
						}
					}
					return req_readcopy(req, list, len);
				}
				case HANDLE_ATA:   case HANDLE_ATA+1:
				case HANDLE_ATA+2: case HANDLE_ATA+3:
					if (req->offset < 0) return -1;
					char buf[512];
					uint32_t sector = req->offset / 512;
					size_t len = min(req->output.len, 512 - ((size_t)req->offset & 511));
					ata_read(id - HANDLE_ATA, sector, buf);
					virt_cpy_to(req->caller->pages, req->output.buf, buf, len);
					return len;
				default: panic_invalid_state();
			}

		case VFSOP_CLOSE: return 0;
		case VFSOP_GETSIZE: return -ENOSYS;

		default: panic_invalid_state();
	}
}

static void accept(struct vfs_request *req) {
	if (req->caller) {
		vfsreq_finish_short(req, handle(req));
	} else {
		vfsreq_finish_short(req, -1);
	}
}

static bool is_ready(struct vfs_backend __attribute__((unused)) *self) {
	return true;
}

static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
void vfs_root_init(void) { vfs_mount_root_register("", &backend); }
