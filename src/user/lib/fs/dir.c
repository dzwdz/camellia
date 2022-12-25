#include <camellia/syscalls.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <camellia/fs/dir.h>

void dir_start(struct dirbuild *db, long offset, char *buf, size_t buflen) {
	db->offset = offset;
	db->buf = buf;
	db->bpos = 0;
	db->blen = buflen;
	db->error = 0;

	// TODO decide how negative directory offsets should be handled
	if (offset < 0) db->error = -ENOSYS;
}

bool dir_append(struct dirbuild *db, const char *name) {
	return dir_appendl(db, name, strlen(name));
}

bool dir_appendl(struct dirbuild *db, const char *name, size_t len) {
	if (db->error) return true;
	if (len > (size_t)LONG_MAX) {
		db->error = -1;
		return true;
	}

	len++; // account for the null byte

	if (db->offset < (long)len) {
		name += db->offset;
		len  -= db->offset;
		db->offset = 0;

		if (db->buf) {
			// TODO no buffer overrun check
			memcpy(db->buf + db->bpos, name, len - 1);
			db->buf[db->bpos + len - 1] = '\0';
		}
		db->bpos += len;
	} else {
		db->offset -= len;
	}
	return false;
}

bool dir_append_from(struct dirbuild *db, handle_t h) {
	if (db->error) return true;
	if (db->buf && db->bpos == db->blen) return false;

	int ret;
	if (db->buf) {
		ret = _syscall_read(h, db->buf + db->bpos, db->blen - db->bpos, db->offset);
		if (ret < 0) {
			db->error = ret;
			return true;
		} else if (ret > 0) {
			/* not eof */
			db->offset = 0;
			db->bpos += ret;
			return false;
		} /* else ret == 0, EOF, need getsize */
	}

	ret = _syscall_getsize(h);
	if (ret < 0) {
		db->error = ret;
		return true;
	}
	if (db->offset < ret) {
		/* should only occur when !buf, otherwise leaks previous data from buf.
		 * TODO consider impact */
		db->bpos += ret - db->offset;
		db->offset = 0;
	} else {
		db->offset -= ret;
	}
	return false;
}

long dir_finish(struct dirbuild *db) {
	return db->error ? db->error : db->bpos;
}
