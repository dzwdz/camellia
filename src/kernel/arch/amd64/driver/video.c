#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/driver/video.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>
#include <shared/printf.h>

static GfxInfo fb;
static char namebuf[64];
static size_t namelen;

enum {
	H_ROOT,
	H_FB,
};

static int handle(VfsReq *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (!req->input.kern) panic_invalid_state();
			if (req->input.len == 1) {
				return H_ROOT;
			} else if (req->input.len == namelen + 1 && !memcmp(req->input.buf_kern + 1, namebuf, namelen)) {
				return H_FB;
			} else {
				return -1;
			}

		case VFSOP_READ:
			if (req->id == H_ROOT) {
				return req_readcopy(req, namebuf, namelen + 1);
			} else {
				return req_readcopy(req, fb.b, fb.size);
			}

		case VFSOP_WRITE:
			if ((long __force)req->id != H_FB) {
				return -1;
			}
			fs_normslice(&req->offset, &req->input.len, fb.size, false);
			/* parial writes ignored */
			pcpy_from(req->caller, fb.b + req->offset, req->input.buf, req->input.len);
			return req->input.len;

		case VFSOP_GETSIZE:
			return fb.size;

		default:
			return -1;
	}
}

static void accept(VfsReq *req) {
	if (req->caller) {
		vfsreq_finish_short(req, handle(req));
	} else {
		vfsreq_finish_short(req, -1);
	}
}

void video_init(GfxInfo fb_) {
	fb = fb_;
	snprintf(namebuf, sizeof namebuf, "%ux%ux%u", fb.width, fb.height, fb.bpp);
	namelen = strlen(namebuf);
	vfs_root_register("/video", accept);
}
