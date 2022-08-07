#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/panic.h>
#include <kernel/ring.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static volatile uint8_t backlog_buf[64];
static volatile ring_t backlog = {(void*)backlog_buf, sizeof backlog_buf, 0, 0};

static void accept(struct vfs_request *req);
static bool is_ready(struct vfs_backend *self);

static struct vfs_request *kb_queue = NULL;
static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
void ps2_init(void) { vfs_mount_root_register("/ps2", &backend); }


void ps2_irq(void) {
	ring_put1b((void*)&backlog, port_in8(0x60));
	if (kb_queue) {
		accept(kb_queue);
		kb_queue = kb_queue->postqueue_next;
		vfs_backend_tryaccept(&backend);
	}
}

enum {
	H_ROOT,
	H_KB,
};

static void accept(struct vfs_request *req) {
	// when you fix something here go also fix it in the COM1 driver
	int ret;
	switch (req->type) {
		case VFSOP_OPEN:
			if (!req->input.kern) panic_invalid_state();
			if (req->input.len == 1) {
				vfsreq_finish_short(req, H_ROOT);
			} else if (req->input.len == 3 && !memcmp(req->input.buf_kern, "/kb", 3)) {
				vfsreq_finish_short(req, H_KB);
			} else {
				vfsreq_finish_short(req, -1);
			}
			break;
		case VFSOP_READ:
			if ((long __force)req->id == H_ROOT) {
				const char data[] = "kb";
				ret = req_readcopy(req, data, sizeof data);
				vfsreq_finish_short(req, ret);
			} else if ((long __force)req->id == H_KB) {
				if (ring_size((void*)&backlog) == 0) {
					/* nothing to read, join queue */
					assert(!req->postqueue_next);
					struct vfs_request **slot = &kb_queue;
					while (*slot) slot = &(*slot)->postqueue_next;
					*slot = req;
				} else if (req->caller) {
					if (ret < 0) ret = 0;
					ret = ring_to_virt((void*)&backlog, req->caller->pages, req->output.buf, req->output.len);
					vfsreq_finish_short(req, ret);
				} else {
					vfsreq_finish_short(req, -1);
				}
			} else panic_invalid_state();
			break;
		default:
			vfsreq_finish_short(req, -1);
			break;
	}
}

static bool is_ready(struct vfs_backend __attribute__((unused)) *self) {
	return true;
}
