#pragma once
#include <stdbool.h>
#include <stddef.h>

/** Reduce a path to its simplest form.
 * *in and *out can't overlap unless they're equal. Then, the path is modified
 * in-place.
 *
 * @return length of the string in *out, always less than len. Negative if the path was invalid.
 */
int path_simplify(const char *in, char *out, size_t len);
