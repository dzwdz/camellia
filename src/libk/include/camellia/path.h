#pragma once
#include <stddef.h>

#define PATH_MAX 512

/** Reduce a path to its simplest form.
 * Kinds of invalid paths:
 *  - relative - "" "a" "./a"
 *  - going behind the root directory - "/../"
 *
 * @return On success, length of the string in *out, <= len. 0 if the path was invalid.
 *
 * returns an unsigned type because:
 * 1. valid paths always return at least 1, for the initial slash
 * 2. it makes it easier to assign the result to an unsigned variable and check for error
 */
size_t path_simplify(const char *in, char *out, size_t len);
