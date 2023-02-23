#include <assert.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool verbose = false;

#define verbosef(...) do { if (verbose) printf(__VA_ARGS__); } while (0)
#define eprintf(fmt, ...) fprintf(stderr, "iochk: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)


void check(hid_t h) {
	const size_t buflen = 4096;
	const size_t offsets[] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 16, 32, 64, 128, 256,
		512, 1024, 2048,
	};
	char *buflast = malloc(buflen);
	char *bufcur = malloc(buflen);
	if (!buflast || !bufcur) {
		eprintf("out of memory");
		goto end;
	}

	long offlast = 0;
	long retlast = _sys_read(h, buflast, buflen, offlast);
	if (retlast < 0) {
		eprintf("error %ld when reading at offset %ld", retlast, offlast);
		goto end;
	}

	for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
		char *tmp;
		long offcur = offsets[i];
		long diff = offcur - offlast;
		assert(diff >= 0);
		if (retlast < diff) break;

		long retcur = _sys_read(h, bufcur, buflen, offcur);
		if (retcur < 0) {
			eprintf("error %ld when reading at offset %ld", retlast, offcur);
			break;
		}
		if (retcur < retlast + offlast - offcur) {
			verbosef("warn: unexpected ret %ld < %ld + %ld - %ld\n", retcur, retlast, offlast, offcur);
		}
		if (memcmp(buflast + diff, bufcur, retlast - diff)) {
			eprintf("inconsistent read from offsets %ld and %ld", offlast, offcur);
		}

		offlast = offcur;
		retlast = retcur;
		tmp     = bufcur;
		bufcur  = buflast;
		buflast = tmp;
	}

	// TODO check negative offsets

end:
	free(buflast);
	free(bufcur);
}

int main(int argc, char **argv) {
	int c;
	while ((c = getopt(argc, argv, "v")) != -1) {
		switch (c) {
			case 'v':
				verbose = true;
				break;
			default:
				return 1;
		}
	}
	if (optind >= argc) {
		eprintf("no files given");
		return 1;
	}
	for (; optind < argc; optind++) {
		const char *path = argv[optind];
		verbosef("checking %s...\n", path);
		hid_t h = camellia_open(path, OPEN_READ);
		if (h < 0) {
			eprintf("couldn't open %s", path);
			continue;
		}
		check(h);
		close(h);
	}
	return 0;
}
