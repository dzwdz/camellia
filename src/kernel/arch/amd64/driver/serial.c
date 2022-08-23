#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/mem/virt.h>
#include <kernel/ring.h>
#include <kernel/panic.h>
#include <stdint.h>

static volatile uint8_t backlog_buf[64];
static volatile ring_t backlog = {(void*)backlog_buf, sizeof backlog_buf, 0, 0};

static const int COM1 = 0x3f8;

static void accept(struct vfs_request *req);

static struct vfs_request *blocked_on = NULL;
static struct vfs_backend backend = BACKEND_KERN(accept);
void serial_init(void) { vfs_mount_root_register("/com1", &backend); }


static void serial_selftest(void) {
	char b = 0x69;
	port_out8(COM1 + 4, 0b00011110); // enable loopback mode
	port_out8(COM1, b);
	assert(port_in8(COM1) == b);
}

void serial_preinit(void) {
	// see https://www.sci.muni.cz/docs/pc/serport.txt
	// set baud rate divisor
	port_out8(COM1 + 3, 0b10000000); // enable DLAB
	port_out8(COM1 + 0, 0x01);       // divisor = 1 (low  byte)
	port_out8(COM1 + 1, 0x00);       //             (high byte)

	port_out8(COM1 + 3, 0b00000011); // 8 bits, no parity, one stop bit
	port_out8(COM1 + 1, 0x01);       // enable the Data Ready IRQ
	port_out8(COM1 + 2, 0b11000111); // enable FIFO with 14-bit trigger level (???)

	serial_selftest();

	port_out8(COM1 + 4, 0b00001111); // enable everything in the MCR
}


void serial_irq(void) {
	ring_put1b((void*)&backlog, port_in8(COM1));
	if (blocked_on) {
		accept(blocked_on);
		blocked_on = blocked_on->postqueue_next;
		vfs_backend_tryaccept(&backend);
	}
}


static void serial_putchar(char c) {
	while ((port_in8(COM1 + 5) & 0x20) == 0); // wait for THRE
	port_out8(COM1, c);
}

void serial_write(const char *buf, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putchar(buf[i]);
}


static void accept(struct vfs_request *req) {
	int ret;
	bool valid;
	switch (req->type) {
		case VFSOP_OPEN:
			valid = req->input.len == 0;
			vfsreq_finish_short(req, valid ? 0 : -1);
			break;
		case VFSOP_READ:
			if (ring_used((void*)&backlog) == 0) {
				/* nothing to read, join queue */
				assert(!req->postqueue_next);
				struct vfs_request **slot = &blocked_on;
				while (*slot) slot = &(*slot)->postqueue_next;
				*slot = req;
			} else if (req->caller) {
				ret = ring_to_virt((void*)&backlog, req->caller->pages, req->output.buf, req->output.len);
				// TODO output.len can overflow here
				vfsreq_finish_short(req, ret);
			} else {
				vfsreq_finish_short(req, -1);
			}
			break;
		case VFSOP_WRITE:
			if (req->caller && !req->flags) {
				struct virt_iter iter;
				virt_iter_new(&iter, req->input.buf, req->input.len,
						req->caller->pages, true, false);
				while (virt_iter_next(&iter))
					serial_write(iter.frag, iter.frag_len);
				ret = iter.prior;
			} else ret = -1;
			vfsreq_finish_short(req, ret);
			break;
		default:
			vfsreq_finish_short(req, -1);
			break;
	}
}
