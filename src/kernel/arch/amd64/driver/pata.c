#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/ata.h>
#include <kernel/arch/amd64/driver/pata.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static const int root_id = 100;

static void accept(struct vfs_request *req);
void pata_init(void) {
	ata_init();
	vfs_root_register("/ata", accept);
}

static void accept(struct vfs_request *req) {
	int ret;
	long id = (long __force)req->id;
	char wbuf[4096];
	size_t len;
	switch (req->type) {
		case VFSOP_OPEN:
			     if (reqpathcmp(req, "/"))  ret = root_id;
			else if (reqpathcmp(req, "/0")) ret = 0;
			else if (reqpathcmp(req, "/1")) ret = 1;
			else if (reqpathcmp(req, "/2")) ret = 2;
			else if (reqpathcmp(req, "/3")) ret = 3;
			else ret = -ENOENT;
			// TODO don't allow opening nonexistent drives
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
			len = min(req->output.len, sizeof wbuf);
			ata_read(id, wbuf, len, req->offset);
			pcpy_to(req->caller, req->output.buf, wbuf, len);
			vfsreq_finish_short(req, len);
			break;

		case VFSOP_WRITE:
			if (id == root_id) {
				vfsreq_finish_short(req, -EACCES);
				break;
			}
			fs_normslice(&req->offset, &req->input.len, ata_size(id), false);
			len = min(req->input.len, sizeof wbuf);
			if (len != 0) {
				len = pcpy_from(req->caller, wbuf, req->input.buf, len);
				ata_write(id, wbuf, len, req->offset);
			}
			vfsreq_finish_short(req, len);
			break;

		case VFSOP_GETSIZE:
			if (id == root_id) {
				panic_unimplemented();
			}
			vfsreq_finish_short(req, ata_size(id));
			break;

		default:
			vfsreq_finish_short(req, -ENOSYS);
			break;
	}
}
