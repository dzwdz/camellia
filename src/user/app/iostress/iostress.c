#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

int main(int argc, char **argv) {
	long num_runs = 4;
	long num_calls = 512;
	long num_bytes = 1;
	uint64_t *results;
	char *inbuf;

	if (argc > 1) num_runs  = strtol(argv[1], NULL, 0);
	if (argc > 2) num_calls = strtol(argv[2], NULL, 0);
	if (argc > 3) num_bytes = strtol(argv[3], NULL, 0);
	if (argc > 4 || num_runs == 0 || num_calls == 0) {
		fprintf(stderr, "usage: %s [num_runs] [num_calls] [num_bytes]\n", argv[0]);
		return 1;
	}

	results = malloc(sizeof(*results) * num_runs);
	inbuf = malloc(num_bytes);
	memset(inbuf, '.', num_bytes);

	for (long i = 0; i < num_runs; i++) {
		uint64_t time = __rdtsc();
		for (long j = 0; j < num_calls; j++)
			_sys_write(1, inbuf, num_bytes, -1, 0);
		results[i] = __rdtsc() - time;
		_sys_write(1, "\n", 1, -1, 0);
	}

	uint64_t total = 0;
	for (long i = 0; i < num_runs; i++) {
		uint64_t scaled = results[i] / 3000;
		total += scaled;
		fprintf(stderr, "run %u: %u\n", i, scaled);
	}
	fprintf(stderr, "%u calls, %u bytes. avg %u\n", num_calls, num_bytes, total / num_runs);

	return 0;
}
