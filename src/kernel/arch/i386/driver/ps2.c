#include <kernel/arch/i386/driver/ps2.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/vfs/request.h>
#include <shared/container/ring.h>
#include <shared/mem.h>

#define BACKLOG_CAPACITY 64
static volatile uint8_t backlog_buf[BACKLOG_CAPACITY];
static volatile ring_t backlog = {(void*)backlog_buf, BACKLOG_CAPACITY, 0, 0};

static void accept(struct vfs_request *req);
static bool is_ready(struct vfs_backend *self);

static struct vfs_request *blocked_on = NULL;
static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
void ps2_init(void) { vfs_mount_root_register("/ps2", &backend); }


void ps2_recv(uint8_t s) {
	ring_put1b((void*)&backlog, s);
	if (blocked_on) {
		accept(blocked_on);
		blocked_on = NULL;
		vfs_backend_tryaccept(&backend);
	}
}

static void accept(struct vfs_request *req) {
	// when you fix something here go also fix it in the COM1 driver
	static uint8_t buf[32]; // pretty damn stupid
	int ret;
	bool valid;
	switch (req->type) {
		case VFSOP_OPEN:
			valid = req->input.len == 0 && !(req->flags & OPEN_CREATE);
			vfsreq_finish_short(req, valid ? 0 : -1);
			break;
		case VFSOP_READ:
			if (ring_size((void*)&backlog) == 0) {
				// nothing to read
				blocked_on = req;
			} else if (req->caller) {
				ret = clamp(0, req->output.len, sizeof buf);
				ret = ring_get((void*)&backlog, buf, ret);
				virt_cpy_to(req->caller->pages, req->output.buf, buf, ret);
				vfsreq_finish_short(req, ret);
			} else {
				vfsreq_finish_short(req, -1);
			}
			break;
		default:
			vfsreq_finish_short(req, -1);
			break;
	}
}

static bool is_ready(struct vfs_backend __attribute__((unused)) *self) {
	return blocked_on == NULL;
}
