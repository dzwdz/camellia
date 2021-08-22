#include <kernel/vfs/path.h>
#include <kernel/panic.h>

int path_simplify(const char *in, char *out, size_t len) {
	if (len == 0)     return -1; // empty paths are invalid
	if (in[0] != '/') return -1; // so are relative paths

	int depth = 0; // shouldn't be needed!
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
			// the segment is // or /./
			directory = true;
		} else if (seg_len == 2 && in[i + 1] == '.' && in[i + 2] == '.') {
			// the segment is /../
			if (--depth < 0)
				return -1;
			// backtrack to last slash
			while (out[--out_pos] != '/');
		} else {
			// normal segment
			out[out_pos] = '/';
			memcpy(&out[out_pos + 1], &in[i + 1], seg_len);
			out_pos += seg_len + 1;
			depth++;
		}

	}

	/* if we were backtracking, out_pos can become 0. i don't like this,
	 * it feels sloppy. this algorithm should be implemented differently. TODO? */
	if (out_pos == 0)
		out[out_pos++] = '/';

	if (directory) // if the path refers to a directory, append a trailing slash
		if (out[out_pos-1] != '/') // unless it's already there
			out[out_pos++] = '/';

	return out_pos;
}
