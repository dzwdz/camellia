#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/driver/pata.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static const int root_id = 100;

static void accept(struct vfs_request *req);
static struct vfs_backend backend = BACKEND_KERN(accept);

void pata_init(void) {
	ata_init();
	vfs_mount_root_register("/ata", &backend);
}


static bool exacteq(struct vfs_request *req, const char *str) {
	size_t len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

static void accept(struct vfs_request *req) {
	int ret;
	long id = (long __force)req->id;
	switch (req->type) {
		case VFSOP_OPEN:
			if (!req->input.kern) panic_invalid_state();
			ret = -ENOENT;
			if (exacteq(req, "/")) ret = root_id;
			else if (exacteq(req, "/0")) ret = 0;
			else if (exacteq(req, "/1")) ret = 1;
			else if (exacteq(req, "/2")) ret = 2;
			else if (exacteq(req, "/3")) ret = 3;
			vfsreq_finish_short(req, ret);
			break;

		case VFSOP_READ:
			if (id == root_id) {
				// TODO use dirbuild
				char list[8] = {};
				size_t len = 0;
				for (int i = 0; i < 4; i++) {
					if (ata_available(i)) {
						list[len] = '0' + i;
						len += 2;
					}
				}
				vfsreq_finish_short(req, req_readcopy(req, list, len));
				break;
			}
			fs_normslice(&req->offset, &req->output.len, ata_size(id), false);

			char buf[512]; /* stupid */
			uint32_t sector = req->offset / ATA_SECTOR;
			size_t skip = (size_t)req->offset & (ATA_SECTOR - 1);
			size_t len = min(req->output.len, ATA_SECTOR - skip);
			ata_read(id, sector, buf);
			virt_cpy_to(req->caller->pages, req->output.buf, buf + skip, len);
			vfsreq_finish_short(req, len);
			break;

		case VFSOP_WRITE:
			panic_unimplemented();

		case VFSOP_GETSIZE:
			if (id == root_id)
				panic_unimplemented();
			vfsreq_finish_short(req, ata_size(id));
			break;

		default:
			vfsreq_finish_short(req, -ENOSYS);
			break;
	}
}
