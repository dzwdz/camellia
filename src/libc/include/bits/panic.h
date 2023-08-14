#pragma once
#include <stdio.h>
#include <stdlib.h>

#define __libc_panic(...) do { fprintf(stderr, "__libc_panic @ %s:", __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); abort(); } while (0)
