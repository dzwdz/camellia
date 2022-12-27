#include <assert.h>
#include <camellia/fsutil.h>
#include <limits.h>

void fs_normslice(long *restrict offset, size_t *restrict length, size_t max, bool expand)
{
	assert(max <= (size_t)LONG_MAX);

	if (*offset < 0) {
		/* Negative offsets are relative to EOF + 1.
		 * Thus:
		 *   write(-1) writes right after the file ends; atomic append
		 *   write(-n) writes, overriding the last (n-1) bytes
		 *   read(-n)  reads the last (n-1) bytes
		 */
		*offset += max + 1;
		if (*offset < 0) {
			/* cursor went before the file, EOF */
			*length = 0;
			*offset = max;
			goto end;
		}
	}

	if (expand) {
		/* This is a write() to a file with a dynamic size.
		 * We don't care if it goes past the current size, because the
		 * driver can handle expanding it. */
	} else {
		/* This operation can't extend the file, it's either:
		 *  - any read()
		 *  - a write() to a file with a static size (e.g. a framebuffer)
		 * *offset and *length describe a slice of a buffer with length max,
		 * so their sum must not overflow it. */
		if ((size_t)*offset <= max) {
			size_t maxlen = max - (size_t)*offset;
			if (*length > maxlen)
				*length = maxlen;
		} else {
			/* regular EOF */
			*length = 0;
			*offset = max;
			goto end;
		}
	}

end:
	assert(0 <= *offset);
	if (!expand)
		assert(*offset + *length <= max);
}
