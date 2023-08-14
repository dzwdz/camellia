#pragma once
#include <camellia/types.h>
#include <stdbool.h>
#include <stddef.h>

struct dirbuild {
	long offset;
	char *buf;
	long bpos, blen;
	long error;
};

void dir_start(struct dirbuild *db, long offset, char *buf, size_t buflen);
bool dir_append(struct dirbuild *db, const char *name);
bool dir_appendl(struct dirbuild *db, const char *name, size_t len);
bool dir_append_from(struct dirbuild *db, hid_t h);
long dir_finish(struct dirbuild *db);
