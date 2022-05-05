#include <kernel/arch/i386/driver/serial.h>
#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/port_io.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <shared/container/ring.h>
#include <shared/mem.h>
#include <stdint.h>

#define BACKLOG_CAPACITY 64
static volatile uint8_t backlog_buf[BACKLOG_CAPACITY];
static volatile ring_t backlog = {(void*)backlog_buf, BACKLOG_CAPACITY, 0, 0};

static const int COM1 = 0x3f8;

static struct vfs_request *blocked_on = NULL;


static void accept(struct vfs_request *req);
static bool is_ready(struct vfs_backend *self);


static struct vfs_backend backend = BACKEND_KERN(is_ready, accept);
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


static bool serial_ready(void) {
	return ring_size((void*)&backlog) > 0;
}

void serial_irq(void) {
	ring_put1b((void*)&backlog, port_in8(COM1));
	if (blocked_on) {
		accept(blocked_on);
		blocked_on = NULL;
		// TODO vfs_backend_tryaccept
	}
}

static size_t serial_read(char *buf, size_t len) {
	return ring_get((void*)&backlog, buf, len);
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
	static char buf[32];
	int ret;
	switch (req->type) {
		case VFSOP_OPEN:
			vfsreq_finish(req, 0);
			break;
		case VFSOP_READ:
			if (serial_ready()) {
				if (req->caller) {
					// clamp between 0, sizeof buf
					ret = req->output.len;
					if (ret > (int)sizeof buf) ret = sizeof buf;
					if (ret < 0) ret = 0;

					ret = serial_read(buf, ret);
					virt_cpy_to(req->caller->pages, req->output.buf, buf, ret);
				} else ret = -1;
				vfsreq_finish(req, ret);
			} else {
				blocked_on = req;
			}
			break;
		case VFSOP_WRITE:
			if (req->caller) {
				struct virt_iter iter;
				virt_iter_new(&iter, req->input.buf, req->input.len,
						req->caller->pages, true, false);
				while (virt_iter_next(&iter))
					serial_write(iter.frag, iter.frag_len);
				ret = iter.prior;
			} else ret = -1;
			vfsreq_finish(req, ret);
			break;
		default:
			vfsreq_finish(req, -1);
			break;
	}
}

static bool is_ready(struct vfs_backend __attribute__((unused)) *self) { return blocked_on == NULL; }
