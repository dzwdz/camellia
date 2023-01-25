#pragma once

/* That page contains a null-terminated string that describes the process
 * for tools such as ps. It's first allocated in the bootstrap process, and
 * then it's freed on every exec() - just to be immediately reallocated by
 * _start2(). */
static char *const _libc_psdata = (void*)0x10000;
