#pragma once

enum handle_type; // forward declaration for proc.h

#include <camellia/types.h>
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

struct handle {
	enum handle_type type;
	struct vfs_backend *backend; // HANDLE_FILE | HANDLE_FS_FRONT
	void __user *file_id; // only applicable to HANDLE_FILE
	// TODO readable/writeable could be reused for pipes
	bool readable, writeable; /* currently only for HANDLE_FILE */
	struct vfs_request *req; /* HANDLE_FS_REQ */
	struct {
		struct process *queued;
		bool write_end;
		struct handle *sister; // the other end, not included in refcount
	} pipe;

	size_t refcount;
};

struct handle *handle_init(enum handle_type);
void handle_close(struct handle *);
