#pragma once

struct _psdata {
	/* Description of the process, see setprogname.
	 * Assumed to be null terminated. */
	char desc[1024];

	/* Base offset where the executable was loaded. */
	void *base;
};

/* First allocated in bootstrap.
 * Freed on every exec(), just to be immediately reallocated by _start2(). */
static struct _psdata *const _psdata_loc = (void*)0x10000;
