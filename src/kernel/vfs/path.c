#include <kernel/vfs/path.h>
#include <kernel/panic.h>

bool path_simplify(const char *in, char *out, size_t len) {
	if (len == 0)     return false; // empty paths are invalid
	if (in[0] != '/') return false; // so are relative paths

	int depth = 0;
	int seg_len; // the length of the current path segment

	for (int i = 0; i < len; i += seg_len + 1) {
		// TODO implement assert
		if (in[i] != '/') panic();

		seg_len = 0;
		for (int j = i + 1; j < len; j++) {
			if (in[j] == '/') break;
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
			// the depth doesn't change
		} else if (seg_len == 2 && in[i + 1] == '.' && in[i + 2] == '.') {
			// the segment is /../
			if (--depth < 0)
				return false;
		} else {
			// normal segment
			depth++;
		}
	}

	return true;
}
