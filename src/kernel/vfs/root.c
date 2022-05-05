#include <kernel/arch/i386/ata.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>
#include <shared/mem.h>
#include <stdbool.h>

// TODO move to arch/

enum {
	HANDLE_ROOT,
	HANDLE_VGA,
	HANDLE_ATA_ROOT,
	HANDLE_ATA,
	_SKIP = HANDLE_ATA + 4,
};

static bool exacteq(struct vfs_request *req, const char *str) {
	size_t len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

/* truncates the length */
static void req_preprocess(struct vfs_request *req, size_t max_len) {
	if (req->offset < 0) {
		// TODO negative offsets
		req->offset = 0;
	}

	if (req->offset >= capped_cast32(max_len)) {
		req->input.len = 0;
		req->output.len = 0;
		req->offset = max_len;
		return;
	}

	req->input.len  = min(req->input.len,  max_len - req->offset);
	req->output.len = min(req->output.len, max_len - req->offset);

	assert(req->input.len + req->offset <= max_len);
	assert(req->input.len + req->offset <= max_len);
}


static int handle(struct vfs_request *req, bool *ready) {
	assert(req->caller);
	switch (req->type) {
		case VFSOP_OPEN:
			if (exacteq(req, "/"))		return HANDLE_ROOT;
			if (exacteq(req, "/vga"))	return HANDLE_VGA;

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
			switch (req->id) {
				case HANDLE_ROOT: {
					// TODO document directory read format
					const char src[] =
						"vga\0"
						"com1\0"
						"ps2\0"
						"ata/";
					int len = min((size_t) req->output.len, sizeof(src));
					virt_cpy_to(req->caller->pages, req->output.buf, src, len);
					return len;
				}
				case HANDLE_VGA: {
					char *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_to(req->caller->pages, req->output.buf,
							vga + req->offset, req->output.len);
					return req->output.len;
				}
				case HANDLE_ATA_ROOT: {
					// TODO offset
					char list[8] = {};
					size_t len = 0;
					for (int i = 0; i < 4; i++) {
						if (ata_available(i)) {
							list[len] = '0' + i;
							len += 2;
						}
					}
					len = min((size_t) req->output.len, len);
					virt_cpy_to(req->caller->pages, req->output.buf, list, len);
					return len;
				}
				case HANDLE_ATA:   case HANDLE_ATA+1:
				case HANDLE_ATA+2: case HANDLE_ATA+3: {
					if (req->offset < 0) return 0;
					char buf[512];
					uint32_t sector = req->offset / 512;
					size_t len = min(req->output.len, 512 - ((size_t)req->offset & 511));
					ata_read(req->id - HANDLE_ATA, sector, buf);
					virt_cpy_to(req->caller->pages, req->output.buf, buf, len);
					return len;
				}
				default: panic_invalid_state();
			}

		case VFSOP_WRITE:
			switch (req->id) {
				case HANDLE_VGA: {
					void *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_from(req->caller->pages, vga + req->offset,
							req->input.buf, req->input.len);
					return req->input.len;
				}
				default: return -1;
			}

		case VFSOP_CLOSE:
			return 0;

		default: panic_invalid_state();
	}
}

int vfs_root_accept(struct vfs_request *req) {
	if (req->caller) {
		bool ready = true;
		int ret = handle(req, &ready);
		if (ready) {
			return vfsreq_finish(req, ret);
		} else {
			return -1;
		}
	} else {
		return vfsreq_finish(req, -1);
	}
}

bool vfs_root_ready(struct vfs_backend *self) {
	return true;
}
