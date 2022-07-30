#pragma once
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
long dir_finish(struct dirbuild *db);
