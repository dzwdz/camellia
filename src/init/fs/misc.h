#pragma once
#include <stdbool.h>

bool fork2_n_mount(const char *path);

void fs_passthru(const char *prefix);
