#include <stdio.h>
#include <camellia/syscalls.h>
#include <x86intrin.h>

#define NUM_RUNS 4
#define NUM_CALLS 512

int main(void) {
	uint64_t time;
	uint64_t results[8];

	for (int i = 0; i < NUM_RUNS; i++) {
		time = __rdtsc();
		for (int j = 0; j < NUM_CALLS; j++)
			_syscall_write(1, ".", 1, -1, 0);
		_syscall_write(1, "\n", 1, -1, 0);
		results[i] = __rdtsc() - time;
	}

	for (int i = 0; i < NUM_RUNS; i++) {
		printf("run %u: %u\n", i, results[i] / 3000);
	}

	return 0;
}
