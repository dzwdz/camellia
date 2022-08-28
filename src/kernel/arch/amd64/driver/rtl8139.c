#include <kernel/arch/amd64/driver/rtl8139.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/arch/amd64/pci.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/request.h>
#include <stdbool.h>

#define WAIT -1000

static void accept(struct vfs_request *req);
static struct vfs_request *blocked_on = NULL;


enum {
	MAC = 0,
	TXSTATUS0 = 0x10,
	TXSTART0 = 0x20,
	RBSTART = 0x30,
	CMD = 0x37,
	CAPR = 0x38,
	CBR = 0x3A,
	INTRMASK = 0x3C,
	INTRSTATUS = 0x3E,
	TCR = 0x40,
	RCR = 0x44,
	CONFIG1 = 0x52,
};

static uint16_t iobase;

#define buflen_shift 3
#define rxbuf_baselen ((8 * 1024) << buflen_shift)
/* the +16 is apparently required for... something */
static char rxbuf[rxbuf_baselen + 16];
static size_t rxpos;

#define txbuf_len 2048
static char txbuf[4][txbuf_len];

static void rx_irq_enable(bool v) {
	uint16_t mask = 1; /* rx ok */
	port_out16(iobase + INTRMASK, v ? mask : 0);
}

void rtl8139_init(uint32_t bdf) {
	if (iobase) panic_unimplemented(); /* multiple devices */
	iobase = pcicfg_iobase(bdf);

	/* also includes the status, because i have only implemented w32 */
	uint32_t cmd = pcicfg_r32(bdf, PCICFG_CMD);
	cmd |= 1 << 2; /* bus mastering */
	pcicfg_w32(bdf, PCICFG_CMD, cmd);


	port_out8(iobase + CONFIG1, 0); /* power on */

	port_out8(iobase + CMD, 0x10); /* software reset */
	while (port_in8(iobase + CMD) & 0x10);

	assert((long)(void*)rxbuf <= 0xFFFFFFFF);
	port_out32(iobase + RBSTART, (long)(void*)rxbuf);

	port_out32(iobase + TCR, 0);

	uint32_t rcr = 0;
	// rcr |= 1 << 0; /* accept all packets */
	rcr |= 1 << 1; /* accept packets with our mac */
	rcr |= 1 << 2; /* accept multicast */
	rcr |= 1 << 3; /* accept broadcast */
	rcr |= buflen_shift << 11;
	rcr |= 7 << 13; /* no rx threshold, copy whole packets */
	port_out32(iobase + RCR, rcr);

	port_out8(iobase + CMD, 0xC); /* enable RX TX */

	rx_irq_enable(false);

	uint64_t mac = (((uint64_t)port_in32(iobase + MAC + 4) & 0xFFFF) << 32) + port_in32(iobase + MAC);
	kprintf("rtl8139 mac %012x\n", mac);

	vfs_root_register("/eth", accept);
}

void rtl8139_irq(void) {
	uint16_t status = port_in16(iobase + INTRSTATUS);
	if (!(status & 1)) {
		kprintf("bad rtl8139 status 0x%x\n", status);
		panic_unimplemented();
	}
	status &= 1;

	/* bit 0 of cmd - Rx Buffer Empty
	 * not a do while() because sometimes the bit is empty on IRQ. no clue why. */
	while (!(port_in8(iobase + CMD) & 1)) {
		if (!postqueue_pop(&blocked_on, accept)) {
			rx_irq_enable(false);
			break;
		}
	}

	//kprintf("rxpos %x cbr %x\n", rxpos, port_in16(iobase + CBR));
	port_out16(iobase + INTRSTATUS, status);
}

static int try_rx(struct pagedir *pages, void __user *dest, size_t dlen) {
	uint16_t flags, size;
	/* bit 0 - Rx Buffer Empty */
	if (port_in8(iobase + CMD) & 1) return WAIT;

	/* https://github.com/qemu/qemu/blob/04ddcda6a/hw/net/rtl8139.c#L1169 */
	/* https://www.cs.usfca.edu/~cruse/cs326f04/RTL8139D_DataSheet.pdf page 12
	 * bits of interest:
	 *  0 - Receive OK
	 * 14 - Physical Address Matched */
	flags = *(uint16_t*)(rxbuf + rxpos);
	rxpos += 2;
	/* doesn't include the header, includes a 4 byte crc */
	size = *(uint16_t*)(rxbuf + rxpos);
	rxpos += 2;
	if (size == 0) panic_invalid_state();

	// kprintf("packet size 0x%x, flags 0x%x, rxpos %x\n", size, flags, rxpos - 4);
	if (dlen > size) dlen = size;
	if (rxpos + dlen <= rxbuf_baselen) {
		virt_cpy_to(pages, dest, rxbuf + rxpos, dlen);
	} else {
		size_t chunk = rxbuf_baselen - rxpos;
		virt_cpy_to(pages, dest, rxbuf + rxpos, chunk);
		virt_cpy_to(pages, dest + chunk, rxbuf, dlen - chunk);
	}

	rxpos += size;
	rxpos = (rxpos + 3) & ~3;
	while (rxpos >= rxbuf_baselen) rxpos -= rxbuf_baselen;
	/* the device adds the 0x10 back, it's supposed to avoid overflow */
	port_out16(iobase + CAPR, rxpos - 0x10);
	return size;
}

static int try_tx(struct pagedir *pages, const void __user *src, size_t slen) {
	static uint8_t desc = 0;

	if (slen > 0xFFF) return -1;
	if (slen > txbuf_len) return -1;

	uint32_t status = port_in32(iobase + TXSTATUS0 + desc*4);
	if (!(status & (1<<13))) {
		/* can't (?) be caused (and thus, tested) on a vm */
		kprintf("try_tx called with all descriptors full.");
		panic_unimplemented();
	}

	virt_cpy_from(pages, txbuf[desc], src, slen);
	assert((long)(void*)txbuf <= 0xFFFFFFFF);
	port_out32(iobase + TXSTART0  + desc*4, (long)(void*)txbuf[desc]);
	port_out32(iobase + TXSTATUS0 + desc*4, slen);

	desc = (desc + 1) & 3;
	return slen;
}

static void accept(struct vfs_request *req) {
	if (!req->caller) {
		vfsreq_finish_short(req, -1);
		return;
	}
	switch (req->type) {
		long ret;
		case VFSOP_OPEN:
			vfsreq_finish_short(req, req->input.len == 0 ? 0 : -1);
			break;
		case VFSOP_READ:
			ret = try_rx(req->caller->pages, req->output.buf, req->output.len);
			if (ret == WAIT) {
				postqueue_join(&blocked_on, req);
				rx_irq_enable(true);
			} else {
				vfsreq_finish_short(req, ret);
			}
			break;
		case VFSOP_WRITE:
			assert(!req->input.kern);
			vfsreq_finish_short(req, try_tx(req->caller->pages, req->input.buf, req->input.len));
			break;
		default:
			vfsreq_finish_short(req, -1);
			break;
	}
}
