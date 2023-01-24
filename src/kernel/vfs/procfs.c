#include <camellia/errno.h>
#include <kernel/mem/virt.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/procfs.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

enum phandle_type {
	PhDir,
	PhIntr,
};

struct phandle {
	uint32_t gid;
	enum phandle_type type;
};

static struct phandle *openpath(const char *path, size_t len, struct process *root);
static struct process *findgid(uint32_t gid, struct process *root);
static void procfs_accept(struct vfs_request *req);
static void procfs_cleanup(struct vfs_backend *be);
static int isdigit(int c);

static struct phandle *
openpath(const char *path, size_t len, struct process *p)
{
	struct phandle *h;
	enum phandle_type type;
	if (len == 0) return NULL;
	path++, len--;

	while (len && isdigit(*path)) {
		/* parse numerical segment / "directory" name */
		uint32_t cid = 0;
		for (; 0 < len && *path != '/'; path++, len--) {
			char c = *path;
			if (!isdigit(c)) {
				return NULL;
			}
			cid = cid * 10 + *path - '0';
		}
		if (len == 0) return NULL;
		assert(*path == '/');
		path++, len--;

		p = p->child;
		if (!p) return NULL;
		while (p->cid != cid) {
			p = p->sibling;
			if (!p) return NULL;
		}
	}

	/* parse the per-process part */
	if (len == 0) {
		type = PhDir;
	} else if (len == 4 && memcmp(path, "intr", 4) == 0) {
		type = PhIntr;
	} else {
		return NULL;
	}

	h = kmalloc(sizeof *h);
	h->gid = p->globalid;
	h->type = type;
	return h;
}

static struct process *
findgid(uint32_t gid, struct process *root)
{
	for (struct process *p = root; p; p = process_next(p, root)) {
		if (p->globalid == gid) return p;
	}
	return NULL;
}

static void
procfs_accept(struct vfs_request *req)
{
	struct process *root = req->backend->kern.data;
	struct phandle *h = (__force void*)req->id;
	struct process *p;
	char buf[512];
	assert(root);
	if (req->type == VFSOP_OPEN) {
		assert(req->input.kern);
		h = openpath(req->input.buf_kern, req->input.len, root);
		vfsreq_finish_short(req, h ? (long)h : -ENOENT);
		return;
	}
	assert(h);
	p = findgid(h->gid, root);
	if (!p) {
		vfsreq_finish_short(req, -EGENERIC);
		return;
	}

	if (req->type == VFSOP_READ && h->type == PhDir) {
		// TODO port dirbuild to kernel
		int pos = 0;
		if (req->offset != 0) {
			vfsreq_finish_short(req, -ENOSYS);
			return;
		}
		pos += snprintf(buf + pos, 512 - pos, "intr")+1;
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
	} else if (req->type == VFSOP_WRITE && h->type == PhIntr) {
		process_intr(p);
		vfsreq_finish_short(req, req->input.len);
	} else if (req->type == VFSOP_CLOSE) {
		kfree(h);
		vfsreq_finish_short(req, 0);
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

static int
isdigit(int c) {
	return '0' <= c && c <= '9';
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
