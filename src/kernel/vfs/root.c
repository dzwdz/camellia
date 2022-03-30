#include <kernel/arch/i386/ata.h>
#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/root.h>
#include <shared/mem.h>

// TODO move to arch/

enum {
	HANDLE_ROOT,
	HANDLE_TTY,
	HANDLE_VGA,
	HANDLE_PS2,
	HANDLE_ATA_ROOT,
	HANDLE_ATA,
	_SKIP = HANDLE_ATA + 4,
};

static bool exacteq(struct vfs_request *req, const char *str) {
	int len = strlen(str);
	assert(req->input.kern);
	return req->input.len == len && !memcmp(req->input.buf_kern, str, len);
}

/* truncates the length */
static void req_preprocess(struct vfs_request *req, int max_len) {
	// max_len is signed because req->*.len are signed too
	// potential place for VULNs to occur - arbitrary kernel reads etc
	if (req->offset < 0) {
		// TODO negative offsets
		req->offset = 0;
	}

	if (req->offset >= max_len) {
		req->input.len = 0;
		req->output.len = 0;
		req->offset = max_len;
		return;
	}

	if (req->input.len  < 0) req->input.len  = 0;
	if (req->output.len < 0) req->output.len = 0;

	req->input.len  = min(req->input.len,  max_len - req->offset);
	req->output.len = min(req->output.len, max_len - req->offset);

	assert(req->input.len >= 0);
	assert(req->output.len >= 0);

	assert(req->input.len + req->offset <= max_len);
	assert(req->input.len + req->offset <= max_len);
}

int vfs_root_handler(struct vfs_request *req) {
	switch (req->type) {
		case VFSOP_OPEN:
			if (exacteq(req, "/"))		return HANDLE_ROOT;
			if (exacteq(req, "/tty"))	return HANDLE_TTY;
			if (exacteq(req, "/vga"))	return HANDLE_VGA;

			if (exacteq(req, "/ps2"))	return HANDLE_PS2;

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
					const char src[] = "tty\0vga\0ata/";
					if (req->output.len < 0) return 0; // is this needed? TODO make that a size_t or something
					int len = min((size_t) req->output.len, sizeof(src));
					virt_cpy_to(req->caller->pages, req->output.buf, src, len);
					return len;
				}
				case HANDLE_TTY: {
					struct virt_iter iter;
					virt_iter_new(&iter, req->output.buf, req->output.len,
							req->caller->pages, true, false);
					while (virt_iter_next(&iter))
						tty_read(iter.frag, iter.frag_len);
					return iter.prior;
				}
				case HANDLE_VGA: {
					char *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_to(req->caller->pages, req->output.buf,
							vga + req->offset, req->output.len);
					return req->output.len;
				}
				case HANDLE_PS2: {
					uint8_t buf[16];
					size_t len = ps2_read(buf, sizeof buf);
					virt_cpy_to(req->caller->pages, req->output.buf, buf, len);
					return len;
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
					int len = min(req->output.len, 512 - (req->offset & 511));
					ata_read(req->id - HANDLE_ATA, sector, buf);
					virt_cpy_to(req->caller->pages, req->output.buf, buf, len);
					return len;
				}
				default: panic_invalid_state();
			}

		case VFSOP_WRITE:
			switch (req->id) {
				case HANDLE_ROOT: return -1;
				case HANDLE_TTY: {
					struct virt_iter iter;
					virt_iter_new(&iter, req->input.buf, req->input.len,
							req->caller->pages, true, false);
					while (virt_iter_next(&iter))
						tty_write(iter.frag, iter.frag_len);
					return iter.prior;
				}
				case HANDLE_VGA: {
					void *vga = (void*)0xB8000;
					req_preprocess(req, 80*25*2);
					virt_cpy_from(req->caller->pages, vga + req->offset,
							req->input.buf, req->input.len);
					return req->input.len;
				}
				case HANDLE_ATA_ROOT: return -1;
				default: panic_invalid_state();
			}

		default: panic_invalid_state();
	}
}
