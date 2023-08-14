#pragma once
/* the comments are directly pasted into user visible strings.
 * keep them short, don't include " */

#define EGENERIC 1 /* unknown error */
#define EFAULT 2
#define EBADF 3 /* bad file descriptor */
#define EINVAL 4
#define ENOSYS 5 /* unsupported */
#define ERANGE 6
#define ENOMEM 7
#define ENOENT 8
#define ENOTEMPTY 9
#define EACCES 10
#define EMFILE 11 /* all file descriptors taken */
#define ECONNRESET 12
#define EPIPE 13
#define ECHILD 14

#define EISDIR 200
#define ENAMETOOLONG 201
#define ENOTDIR 202
#define ELOOP 203
#define ENOEXEC 204
#define EINTR 205
#define EWOULDBLOCK 206
#define EEXIST 207
#define EAGAIN 208
