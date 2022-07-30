#include <errno.h>
#include <string.h>
#include <user/lib/fs/dir.h>

void dir_start(struct dirbuild *db, long offset, char *buf, size_t buflen) {
	db->offset = offset;
	db->buf = buf;
	db->bpos = 0;
	db->blen = buflen;
	db->error = 0;

	if (offset < 0)
		db->error = -ENOSYS; // TODO
}

bool dir_append(struct dirbuild *db, const char *name) {
	if (db->error) return true;

	long len = strlen(name) + 1;

	if (db->offset < len) {
		name += db->offset;
		len  -= db->offset;
		db->offset = 0;

		// TODO no buffer overrun check
		memcpy(db->buf + db->bpos, name, len);
		db->bpos += len;
	} else {
		db->offset -= len;
	}
	return false;
}

long dir_finish(struct dirbuild *db) {
	return db->error ? db->error : db->bpos;
}
