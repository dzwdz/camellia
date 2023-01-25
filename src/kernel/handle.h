#pragma once
#include <kernel/types.h>
#include <kernel/vfs/mount.h>
#include <kernel/vfs/request.h>
#include <stddef.h>

enum handle_type {
	HANDLE_INVALID = 0,
	HANDLE_FILE,
	HANDLE_PIPE,
	HANDLE_FS_FRONT,
	HANDLE_FS_REQ,
};

struct Handle {
	enum handle_type type;
	VfsBackend *backend; // HANDLE_FILE | HANDLE_FS_FRONT
	void __user *file_id; // only applicable to HANDLE_FILE
	bool readable, writeable; /* HANDLE_FILE | HANDLE_PIPE */
	VfsReq *req; /* HANDLE_FS_REQ */
	struct {
		Proc *queued;
		Handle *sister; // the other end, not included in refcount
	} pipe;

	size_t refcount;
};

Handle *handle_init(enum handle_type);
void handle_close(Handle *);
