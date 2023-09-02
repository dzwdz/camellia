#include <camellia/intr.h>
#include <stdlib.h>

static void intr_null(void) { }

extern void (*volatile _intr)(void);
void intr_set(void (*fn)(void)) {
	_intr = fn ? fn : intr_null;
}

void intr_default(void) {
	exit(-1);
}
