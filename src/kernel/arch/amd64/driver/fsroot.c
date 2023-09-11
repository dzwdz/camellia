#include <camellia/errno.h>
#include <camellia/fsutil.h>
#include <kernel/arch/amd64/driver/driver.h>
#include <kernel/arch/amd64/driver/util.h>
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/util.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

enum {
	Hbase,
	Hdev,
};

static int
get_dev(char *lst)
{
	int len = 0;
	for (VfsMount *m = vfs_mount_seed(); m; m = m->prev) {
		if (m->prefix_len == 0) {
			continue; /* that's us */
		}
		assert(m->prefix_len > 5);
		assert(memcmp(m->prefix, "/dev/", 5) == 0);
		len += m->prefix_len - 5 + 1;
		if (lst) {
			memcpy(lst, m->prefix + 5, m->prefix_len - 5);
			lst += m->prefix_len - 5;
			*lst++ = '\0';
		}
	}
	return len;
}

static long
handle(VfsReq *req)
{
	static char *dev = NULL;
	static int dev_len = 0;
	const char *lst = NULL;
	int len = 0;

	if (!req->caller) return -1;
	if (req->type != VFSOP_OPEN) {
		switch ((uintptr_t __force)req->id) {
		case Hbase:
			lst = "dev/";
			len = strlen(lst) + 1;
			break;
		case Hdev:
			if (!dev) {
				dev_len = get_dev(NULL);
				dev = kmalloc(dev_len);
				get_dev(dev);
			}
			lst = dev;
			len = dev_len;
			break;
		default:
			assert(false);
		}
	}

	switch (req->type) {
	case VFSOP_OPEN:
		if (reqpathcmp(req, "/")) return Hbase;
		if (reqpathcmp(req, "/dev/")) return Hdev;
		return -ENOENT;
	case VFSOP_READ:
		return req_readcopy(req, lst, len);
	case VFSOP_GETSIZE:
		return len;
	default:
		return -ENOSYS;
	}
}

static void
accept(VfsReq *req)
{
	vfsreq_finish_short(req, handle(req));
}

void
vfs_root_init(void)
{
	vfs_root_register("", accept);
}
