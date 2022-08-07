#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/panic.h>
#include <kernel/ring.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static const int PS2 = 0x60;

static volatile uint8_t kb_buf[64];
static volatile ring_t kb_backlog = {(void*)kb_buf, sizeof kb_buf, 0, 0};

static volatile uint8_t mouse_buf[64];
static volatile ring_t mouse_backlog = {(void*)mouse_buf, sizeof mouse_buf, 0, 0};

static void accept(struct vfs_request *req);
static bool is_ready(struct vfs_backend *self);

static struct vfs_request *kb_queue = NULL;
static struct vfs_request *mouse_queue = NULL;
static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);

static void wait_out(void) {
	uint8_t status;
	do {
		status = port_in8(PS2 + 4);
	} while (status & 2);
}

static void wait_in(void) {
	uint8_t status;
	do {
		status = port_in8(PS2 + 4);
	} while (!(status & 1));
}

void ps2_init(void) {
	vfs_mount_root_register("/ps2", &backend);

	uint8_t compaq, ack;
	wait_out();
	port_out8(PS2 + 4, 0x20); /* get compaq status */

	wait_in();
	compaq = port_in8(PS2);
	compaq |= 1 << 1; /* enable IRQ12 */
	compaq &= ~(1 << 5); /* enable mouse clock */

	wait_out();
	port_out8(PS2 + 4, 0x60); /* set compaq status */

	wait_out();
	port_out8(PS2, compaq);

	wait_out();
	port_out8(PS2 + 4, 0xD4);
	wait_out();
	port_out8(PS2, 0xF4); /* packet streaming */
	wait_in();
	ack = port_in8(PS2);
	assert(ack == 0xFA);
}

void ps2_irq(void) {
	for (;;) {
		uint64_t status = port_in8(PS2 + 4);
		if (!(status & 1)) break; /* read while data available */
		if (status & (1 << 5)) {
			ring_put1b((void*)&mouse_backlog, port_in8(PS2));
			if (mouse_queue) {
				accept(mouse_queue);
				mouse_queue = mouse_queue->postqueue_next;
				vfs_backend_tryaccept(&backend);
			}
		} else {
			ring_put1b((void*)&kb_backlog, port_in8(PS2));
			if (kb_queue) {
				accept(kb_queue);
				kb_queue = kb_queue->postqueue_next;
				vfs_backend_tryaccept(&backend);
			}
		}
	}
}

enum {
	H_ROOT,
	H_KB,
	H_MOUSE,
};

static void read_backlog(struct vfs_request *req, ring_t *r, struct vfs_request **queue) {
	if (ring_size(r) == 0) {
		/* nothing to read, join queue */
		assert(!req->postqueue_next);
		while (*queue) queue = &(*queue)->postqueue_next;
		*queue = req;
	} else if (req->caller) {
		int len = req->output.len;
		if (len < 0) len = 0;
		len = ring_to_virt(r, req->caller->pages, req->output.buf, len);
		vfsreq_finish_short(req, len);
	} else {
		vfsreq_finish_short(req, -1);
	}
}

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
			} else if (req->input.len == 6 && !memcmp(req->input.buf_kern, "/mouse", 6)) {
				vfsreq_finish_short(req, H_MOUSE);
			} else {
				vfsreq_finish_short(req, -1);
			}
			break;
		case VFSOP_READ:
			if ((long __force)req->id == H_ROOT) {
				const char data[] = "kb\0mouse";
				ret = req_readcopy(req, data, sizeof data);
				vfsreq_finish_short(req, ret);
			} else if ((long __force)req->id == H_KB) {
				read_backlog(req, (void*)&kb_backlog, &kb_queue);
			} else if ((long __force)req->id == H_MOUSE) {
				read_backlog(req, (void*)&mouse_backlog, &mouse_queue);
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
