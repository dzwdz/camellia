#pragma once
#include <camellia/types.h> // TODO only needed because of handle_t

int fork(void);
int close(handle_t h);
_Noreturn void exit(int);
