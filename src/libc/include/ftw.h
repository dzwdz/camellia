#pragma once
#include <sys/stat.h>

int ftw(const char *dirpath,
		int (*fn)(const char *fpath, const struct stat *sb, int typeflag),
		int nopenfd);
