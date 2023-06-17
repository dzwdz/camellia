#include <camellia/errno.h>
#include <kernel/arch/amd64/driver/util.h> // portable despite the name
#include <kernel/malloc.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/vfs/procfs.h>
#include <kernel/vfs/request.h>
#include <shared/mem.h>

enum phandle_type {
	PhRoot,
	PhDir,
	PhIntr,
	PhMem,
};

struct phandle {
	uint32_t gid;
	enum phandle_type type;
};

static struct phandle *openpath(const char *path, size_t len, Proc *root);
static Proc *findgid(uint32_t gid, Proc *root);
static void procfs_accept(VfsReq *req);
static void procfs_cleanup(VfsBackend *be);
static int isdigit(int c);

static struct phandle *
openpath(const char *path, size_t len, Proc *root)
{
	struct phandle *h;
	enum phandle_type type;
	uint32_t gid = 0;
	if (len == 0) return NULL;
	path++, len--;

	if (len == 0) {
		type = PhRoot;
	} else if (isdigit(*path)) {
		Proc *p;
		uint32_t lid = 0;
		for (; 0 < len && *path != '/'; path++, len--) {
			if (!isdigit(*path)) {
				return NULL;
			}
			lid = lid * 10 + *path - '0';
		}
		if (len == 0) {
			return NULL;
		}
		assert(*path == '/');
		path++, len--;

		if (len == 0) {
			type = PhDir;
		} else if (len == 4 && memcmp(path, "intr", 4) == 0) {
			type = PhIntr;
		} else if (len == 3 && memcmp(path, "mem", 3) == 0) {
			type = PhMem;
		} else {
			return NULL;
		}

		p = proc_ns_byid(root, lid);
		if (!p) {
			return NULL;
		}
		gid = p->globalid;
	} else {
		return NULL;
	}

	h = kmalloc(sizeof *h);
	h->gid = gid;
	h->type = type;
	return h;
}

static Proc *
findgid(uint32_t gid, Proc *root)
{
	for (Proc *p = root; p; p = proc_next(p, root)) {
		if (p->globalid == gid) return p;
	}
	return NULL;
}

static void
procfs_accept(VfsReq *req)
{
	Proc *root = req->backend->kern.data;
	struct phandle *h = (__force void*)req->id;
	Proc *p;
	char buf[512];
	assert(root);
	assert(root->pns == root);

	if (req->type == VFSOP_OPEN) {
		assert(req->input.kern);
		h = openpath(req->input.buf_kern, req->input.len, root);
		vfsreq_finish_short(req, h ? (long)h : -ENOENT);
		return;
	} else if (req->type == VFSOP_CLOSE) {
		assert(h);
		kfree(h);
		vfsreq_finish_short(req, 0);
		return;
	} else {
		assert(h);
	}

	if (h->type != PhRoot) {
		p = findgid(h->gid, root);
		if (!p) {
			vfsreq_finish_short(req, -ENOENT);
			return;
		}
	}

	if (req->type == VFSOP_READ && (h->type == PhDir || h->type == PhRoot)) {
		// TODO port dirbuild to kernel
		int pos = 0;
		if (h->type == PhDir) {
			pos += snprintf(buf + pos, 512 - pos, "intr")+1;
			pos += snprintf(buf + pos, 512 - pos, "mem")+1;
		} else {
			for (Proc *it = root; it; it = proc_ns_next(root, it)) {
				assert(pos < 512);
				pos += snprintf(buf + pos, 512 - pos, "%d/", proc_ns_id(root, it)) + 1;
				if (512 <= pos) {
					vfsreq_finish_short(req, -EGENERIC);
				}
			}
		}
		assert(0 <= pos && (size_t)pos <= sizeof buf);
		vfsreq_finish_short(req, req_readcopy(req, buf, pos));
	} else if (req->type == VFSOP_READ && h->type == PhMem) {
		if (p->pages == NULL || req->caller->pages == NULL) {
			vfsreq_finish_short(req, 0);
			return;
		}
		size_t res = pcpy_bi(
				req->caller, req->output.buf,
				p, (__user void*)req->offset,
				req->output.len
		);
		vfsreq_finish_short(req, res);
	} else if (req->type == VFSOP_WRITE && h->type == PhIntr) {
		proc_intr(p);
		vfsreq_finish_short(req, req->input.len);
	} else {
		vfsreq_finish_short(req, -ENOSYS);
	}
}

static void
procfs_cleanup(VfsBackend *be)
{
	Proc *p = be->kern.data;
	assert(p);
	p->refcount--;
}

static int
isdigit(int c) {
	return '0' <= c && c <= '9';
}

VfsBackend *
procfs_backend(Proc *proc)
{
	VfsBackend *be = kzalloc(sizeof(VfsBackend));
	*be = (VfsBackend) {
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
