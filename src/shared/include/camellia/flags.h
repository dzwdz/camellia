#pragma once

#define MEMFLAG_PRESENT 1
#define MEMFLAG_FINDFREE 2

#define FORK_NOREAP 1
#define FORK_NEWFS 2
#define FORK_SHAREMEM 4
#define FORK_SHAREHANDLE 8

#define WRITE_TRUNCATE 1

#define FSR_DELEGATE 1

#define DUP_SEARCH 1

#define OPEN_READ 1
#define OPEN_WRITE 2
#define OPEN_RW 3
/* not setting OPEN_READ nor OPEN_WRITE works as if OPEN_READ was set, but it also checks the execute bit.
 * same as in plan9. */
#define OPEN_EXEC 0

#define OPEN_READABLE(flags) ((flags & 3) != OPEN_WRITE)
#define OPEN_WRITEABLE(flags) (flags & OPEN_WRITE)

/* Requires OPEN_WRITE to be set, enforced by the kernel.
 * The idea is that if all flags which allow modifying the filesystem state require
 * OPEN_WRITE to be set, filesystem handlers could just check for the OPEN_WRITE flag. */
#define OPEN_CREATE 4


/* special handles */
#define HANDLE_NULLFS -2
#define HANDLE_PROCFS -3
