/*
 * Copyright (c) 2012, 2014, 2021 Jonas 'Sortie' Termansen.
 * Copyright (c) 2021 Juhani 'nortti' Krekelä.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * stdlib/qsort_r.c
 * Sort an array. Implemented using heapsort, which is not a stable sort.
 */

#include <stdbool.h>
#include <stdlib.h>

static void memswap(unsigned char* a, unsigned char* b, size_t size)
{
	unsigned char tmp;
	for ( size_t i = 0; i < size; i++ )
	{
		tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}

static unsigned char* array_index(unsigned char* base,
                                  size_t element_size,
                                  size_t index)
{
	return base + element_size * index;
}

void qsort_r(void* base_ptr,
             size_t num_elements,
             size_t element_size,
             int (*compare)(const void*, const void*, void*),
             void* arg)
{
	unsigned char* base = base_ptr;

	if ( !element_size || num_elements < 2 )
		return;

	// Incrementally left-to-right transform the array into a max-heap, where
	// each element has up to two children that aren't bigger than the element.
	for ( size_t i = 0; i < num_elements; i++ )
	{
		// Grow the heap by inserting another element into it (implicit, done by
		// moving the boundary between the heap and the yet-to-be-processed
		// array elements) and swapping it up the parent chain as long as it's
		// bigger than its parent.
		size_t element = i;
		while ( element > 0 )
		{
			unsigned char* ptr = array_index(base, element_size, element);
			size_t parent = (element - 1) / 2;
			unsigned char* parent_ptr = array_index(base, element_size, parent);
			if ( compare(parent_ptr, ptr, arg) < 0 )
			{
				memswap(parent_ptr, ptr, element_size);
				element = parent;
			}
			else
				break;
		}
	}

	// The array is split into two sections, first the max-heap and then the
	// part of the array that has been sorted.
	// Sorting progresses right-to-left, taking the biggest value of the heap
	// (at its root), swapping it with the last element of the heap, and
	// adjusting the boundary between the two sections such that the old biggest
	// value is now part of the sorted section.
	// After this, the max-heap property is restored by swapping the old last
	// element down as long as it's smaller than one of its children.
	for ( size_t size = num_elements; --size; )
	{
		memswap(array_index(base, element_size, size), base, element_size);

		size_t first_without_left = size / 2;
		size_t first_without_right = (size - 1) / 2;

		size_t element = 0;
		while ( element < size )
		{
			unsigned char* ptr = array_index(base, element_size, element);

			size_t left = 2 * element + 1;
			unsigned char* left_ptr = NULL;
			bool left_bigger = false;
			if ( element < first_without_left )
			{
				left_ptr = array_index(base, element_size, left);
				left_bigger = compare(ptr, left_ptr, arg) < 0;
			}

			size_t right = 2 * element + 2;
			unsigned char* right_ptr = NULL;
			bool right_bigger = false;
			if ( element < first_without_right )
			{
				right_ptr = array_index(base, element_size, right);
				right_bigger = compare(ptr, right_ptr, arg) < 0;
			}

			if ( left_bigger && right_bigger )
			{
				// If both the left and right child are bigger than the element,
				// then swap the element with whichever of the left and right
				// child is bigger.
				if ( compare(left_ptr, right_ptr, arg) < 0 )
				{
					memswap(ptr, right_ptr, element_size);
					element = right;
				}
				else
				{
					memswap(ptr, left_ptr, element_size);
					element = left;
				}
			}
			else if ( left_bigger )
			{
				memswap(ptr, left_ptr, element_size);
				element = left;
			}
			else if ( right_bigger )
			{
				memswap(ptr, right_ptr, element_size);
				element = right;
			}
			else
				break;
		}
	}
}

