#pragma once
#include <camellia/syscalls.h>
#include <stdbool.h>

void cmd_cat_ls(const char *args, bool ls);
void cmd_hexdump(const char *args);
void cmd_touch(const char *args);
