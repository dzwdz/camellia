#pragma once
#include <stdbool.h>
#include <stddef.h>

/** Reduce a path to its simplest form.
 * *in and *out can't overlap unless they're equal. Then, the path is modified
 * in-place.
 *
 * @return Was the path valid? If this is false, *out is undefined
 */
bool path_simplify(const char *in, char *out, size_t len);
