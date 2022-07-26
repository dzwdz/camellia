#include <kernel/panic.h>
#include <kernel/vfs/path.h>
#include <shared/mem.h>

int path_simplify(const char *in, char *out, size_t len) {
	if (len == 0)     return -1; // empty paths are invalid
	if (in[0] != '/') return -1; // so are relative paths

	int seg_len; // the length of the current path segment
	int out_pos = 0;
	bool directory = 0;

	for (size_t i = 0; i < len; i += seg_len + 1) {
		assert(in[i] == '/');

		seg_len = 0;
		directory = false;
		for (size_t j = i + 1; j < len; j++) {
			if (in[j] == '/') {
				directory = true;
				break;
			}
			seg_len++;
		}

		/* example iteration, illustrated with terrible ASCII art
		 *
		 *          |i=5 |next i = i + seg_len + 1 = 10
		 *          v    v
		 *     /some/path/asdf
		 *           |--|
		 *           seg_len = 4
		 *           (segment starts at i+1) */

		if (seg_len == 0 || (seg_len == 1 && in[i + 1] == '.')) {
			/*  // or /./  */
			directory = true;
		} else if (seg_len == 2 && in[i + 1] == '.' && in[i + 2] == '.') {
			/*  /../  */
			directory = true;

			/* try to backtrack to the last slash */
			while (--out_pos >= 0 && out[out_pos] != '/');
			if (out_pos < 0) return -1;
		} else {
			/* a normal segment, e.g. /asdf/ */
			out[out_pos] = '/';
			memcpy(&out[out_pos + 1], &in[i + 1], seg_len);
			out_pos += seg_len + 1;
		}

	}

	/* paths to directories should have a trailing slash */
	if (directory) out[out_pos++] = '/';

	return out_pos;
}
