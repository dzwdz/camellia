#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/driver/video.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/vfs/request.h>

static struct fb_info fb;

enum {
	H_ROOT,
	H_FB,
};

static int handle(struct vfs_request *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (req->input.len == 1) {
				return H_ROOT;
			} else if (req->input.len == 2 && req->input.kern && req->input.buf_kern[1] == 'b') {
				return H_FB;
			} else {
				return -1;
			}

		case VFSOP_READ:
			if (req->id == H_ROOT) {
				// TODO list available modes, e.g /640x480x24
				const char src[] = "b";
				return req_readcopy(req, src, sizeof src);
			} else {
				return req_readcopy(req, fb.b, fb.pitch * fb.height);
			}

		case VFSOP_WRITE:
			if ((long)req->id != H_FB) {
				return -1;
			}
			fs_normslice(&req->offset, &req->input.len, fb.pitch * fb.height, false);
			if (!virt_cpy_from(req->caller->pages, fb.b + req->offset,
					req->input.buf, req->input.len))
			{
				panic_invalid_state();
				return -EFAULT;
			}
			return req->input.len;

		default:
			return -1;
	}
}

static void accept(struct vfs_request *req) {
	if (req->caller) {
		vfsreq_finish_short(req, handle(req));
	} else {
		vfsreq_finish_short(req, -1);
	}
}

static bool is_ready(struct vfs_backend *self) {
	(void)self;
	return true;
}

static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
void video_init(struct fb_info fb_) {
	fb = fb_;
	vfs_mount_root_register("/video", &backend);
}
