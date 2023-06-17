#pragma once
#include <stdio.h>

struct dirent {
	ino_t d_ino;
	char d_name[256]; /* NAME_MAX + 1 */
};

typedef struct {
	FILE *fp;
	struct dirent dent;
} DIR;

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
