#pragma once
#include <stdbool.h>
#include <stddef.h>

/** Normalizes the offset and length passed to a fs into safe values.
 *
 * @param expand Can this operation expand the target file?
 *                true  if writing to a file with an adjustable size
 *                false if reading any sort of file
 *                      or writing to a file with static size
 */
void fs_normslice(long *restrict offset, size_t *restrict length, size_t max, bool expand);
