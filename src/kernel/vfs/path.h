#pragma once
#include <stddef.h>

#define PATH_MAX 512

/** Reduce a path to its simplest form.
 *
 * @return length of the string in *out, always less than len. Negative if the path was invalid.
 */
int path_simplify(const char *in, char *out, size_t len);
