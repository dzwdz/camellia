/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 * Modified by dzwdz.
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
 * stdio/vfscanf.c
 * Input format conversion.
 *
 * stdio/vfscanf_unlocked.c
 * Input format conversion.
 */

#include <errno.h>
#include <stdio.h>

static int wrap_fgetc(void* fp)
{
	return fgetc((FILE*) fp);
}

static int wrap_ungetc(int c, void* fp)
{
	return ungetc(c, (FILE*) fp);
}

int vfscanf(FILE* fp, const char* format, va_list ap)
{
	// if ( !(fp->flags & _FILE_READABLE) )
	// 	return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	return vcbscanf(fp, wrap_fgetc, wrap_ungetc, format, ap);
}
