#pragma once

void thread_create(int flags, void (*fn)(void*), void *arg);
_Noreturn void chstack(void *arg, void (*fn)(void*), void *esp);
