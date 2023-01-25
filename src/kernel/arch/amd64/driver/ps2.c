#include <camellia/errno.h>
#include <kernel/arch/amd64/driver/driver.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/interrupts.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static const int PS2 = 0x60;

static volatile uint8_t kb_buf[64];
static volatile ring_t kb_backlog = {(void*)kb_buf, sizeof kb_buf, 0, 0};

static volatile uint8_t mouse_buf[64];
static volatile ring_t mouse_backlog = {(void*)mouse_buf, sizeof mouse_buf, 0, 0};

static void accept(VfsReq *req);
static void ps2_irq(void);

static VfsReq *kb_queue = NULL;
static VfsReq *mouse_queue = NULL;

static void wait_out(void) {
	while ((port_in8(PS2 + 4) & 2) != 0);
}

static void wait_in(void) {
	while ((port_in8(PS2 + 4) & 1) == 0);
}

void ps2_init(void) {
	uint8_t compaq;
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
	if (port_in8(PS2) != 0xFA) { /* check ACK */
		panic_unimplemented();
	}

	irq_fn[IRQ_PS2KB] = ps2_irq;
	irq_fn[IRQ_PS2MOUSE] = ps2_irq;

	vfs_root_register("/ps2", accept);
}

static void ps2_irq(void) {
	for (;;) {
		uint64_t status = port_in8(PS2 + 4);
		if (!(status & 1)) break; /* read while data available */
		if (status & (1 << 5)) {
			ring_put1b((void*)&mouse_backlog, port_in8(PS2));
			postqueue_ringreadall(&mouse_queue, (void*)&mouse_backlog);
		} else {
			ring_put1b((void*)&kb_backlog, port_in8(PS2));
			postqueue_ringreadall(&kb_queue, (void*)&kb_backlog);
		}
	}
}

enum {
	H_ROOT,
	H_KB,
	H_MOUSE,
};

static void accept(VfsReq *req) {
	// when you fix something here go also fix it in the COM1 driver
	int ret;
	switch (req->type) {
		case VFSOP_OPEN:
			     if (reqpathcmp(req, "/"))      ret = H_ROOT;
			else if (reqpathcmp(req, "/kb"))    ret = H_KB;
			else if (reqpathcmp(req, "/mouse")) ret = H_MOUSE;
			else                                ret = -ENOENT;
			vfsreq_finish_short(req, ret);
			break;
		case VFSOP_READ:
			if ((long __force)req->id == H_ROOT) {
				const char data[] = "kb\0mouse";
				ret = req_readcopy(req, data, sizeof data);
				vfsreq_finish_short(req, ret);
			} else if ((long __force)req->id == H_KB) {
				postqueue_join(&kb_queue, req);
				postqueue_ringreadall(&kb_queue, (void*)&kb_backlog);
			} else if ((long __force)req->id == H_MOUSE) {
				postqueue_join(&mouse_queue, req);
				postqueue_ringreadall(&mouse_queue, (void*)&mouse_backlog);
			} else panic_invalid_state();
			break;
		default:
			vfsreq_finish_short(req, -1);
			break;
	}
}
