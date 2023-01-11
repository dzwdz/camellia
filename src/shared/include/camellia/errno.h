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

#define EISDIR 200
#define ENAMETOOLONG 201
