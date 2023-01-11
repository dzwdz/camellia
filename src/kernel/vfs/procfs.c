#include <camellia/errno.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/procfs.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

static uint32_t openpath(const char *path, size_t len, struct process *root);
static struct process *findgid(uint32_t gid, struct process *root);
static void procfs_accept(struct vfs_request *req);
static void procfs_cleanup(struct vfs_backend *be);

static uint32_t
openpath(const char *path, size_t len, struct process *p)
{
	if (len == 0) return 0;
	path++, len--;

	while (len) {
		uint32_t cid = 0;
		for (; 0 < len && *path != '/'; path++, len--) {
			char c = *path;
			if (!('0' <= c && c <= '9')) {
				return 0;
			}
			cid = cid * 10 + *path - '0';
		}
		if (len == 0) {
			return 0;
		}
		assert(*path == '/');
		path++, len--;

		p = p->child;
		if (!p) return 0;
		while (p->cid != cid) {
			p = p->sibling;
			if (!p) return 0;
		}
	}
	return p->globalid;
}

static struct process *
findgid(uint32_t gid, struct process *root)
{
	for (struct process *p = root; p; p = process_next(p)) {
		if (p->globalid == gid) return p;
	}
	return NULL;
}

static void
procfs_accept(struct vfs_request *req)
{
	struct process *root = req->backend->kern.data;
	struct process *p;
	char buf[512];
	assert(root);
	if (req->type == VFSOP_OPEN) {
		int gid;
		assert(req->input.kern);
		gid = openpath(req->input.buf_kern, req->input.len, root);
		vfsreq_finish_short(req, gid == 0 ? -ENOENT : gid);
		return;
	}
	p = findgid((uintptr_t)req->id, root);
	if (!p) {
		vfsreq_finish_short(req, -EGENERIC);
		return;
	}

	if (req->type == VFSOP_READ) {
		// TODO port dirbuild to kernel
		int pos = 0;
		if (req->offset != 0) {
			vfsreq_finish_short(req, -ENOSYS);
			return;
		}
		for (struct process *iter = p->child; iter; iter = iter->sibling) {
			assert(pos < 512);
			// processes could possibly be identified by unique identifiers instead
			// e.g. an encrypted gid, or just a randomly generated one
			// con: would require bringing in a crypto library
			pos += snprintf(buf + pos, 512 - pos, "%d/", iter->cid) + 1;
			if (512 <= pos) {
				vfsreq_finish_short(req, -1);
			}
		}
		assert(0 <= pos && (size_t)pos <= sizeof buf);
		virt_cpy_to(req->caller->pages, req->output.buf, buf, pos);
		vfsreq_finish_short(req, pos);
	} else {
		vfsreq_finish_short(req, -ENOSYS);
	}
}

static void
procfs_cleanup(struct vfs_backend *be)
{
	struct process *p = be->kern.data;
	assert(p);
	p->refcount--;
}

struct vfs_backend *
procfs_backend(struct process *proc)
{
	struct vfs_backend *be = kzalloc(sizeof(struct vfs_backend));
	*be = (struct vfs_backend) {
		.is_user = false,
		.provhcnt = 1,
		.usehcnt = 1,
		.kern.accept = procfs_accept,
		.kern.data = proc,
		.kern.cleanup = procfs_cleanup,
	};
	proc->refcount++;
	assert(proc->refcount); /* overflow */
	return be;
}
