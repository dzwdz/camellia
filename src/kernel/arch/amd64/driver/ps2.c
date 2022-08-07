#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/panic.h>
#include <kernel/ring.h>
#include <kernel/vfs/request.h>

static volatile uint8_t backlog_buf[64];
static volatile ring_t backlog = {(void*)backlog_buf, sizeof backlog_buf, 0, 0};

static void accept(struct vfs_request *req);
static bool is_ready(struct vfs_backend *self);

static struct vfs_request *blocked_on = NULL;
static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
void ps2_init(void) { vfs_mount_root_register("/ps2", &backend); }


void ps2_irq(void) {
	ring_put1b((void*)&backlog, port_in8(0x60));
	if (blocked_on) {
		accept(blocked_on);
		blocked_on = NULL;
		vfs_backend_tryaccept(&backend);
	}
}

static void accept(struct vfs_request *req) {
	// when you fix something here go also fix it in the COM1 driver
	int ret;
	bool valid;
	switch (req->type) {
		case VFSOP_OPEN:
			valid = req->input.len == 0;
			vfsreq_finish_short(req, valid ? 0 : -1);
			break;
		case VFSOP_READ:
			if (ring_size((void*)&backlog) == 0) {
				// nothing to read
				blocked_on = req;
			} else if (req->caller) {
				if (ret < 0) ret = 0;
				ret = ring_to_virt((void*)&backlog, req->caller->pages, req->output.buf, req->output.len);
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
