#include <_proc.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elfload.h>

int main(int argc, char **argv, char **envp);

__attribute__((visibility("hidden")))
extern char __executable_start[];

const char *shortname(const char *path) {
	if (!path) return "unknown program";
	const char *slash = strrchr(path, '/');
	if (slash) return slash + 1;
	return path;
}

void intr_trampoline(void); /* intr.s */

_Noreturn void _start2(struct execdata *ed) {
	const char *progname;
	elf_selfreloc();

	/* done first so it isn't allocated elsewhere by accident */
	_sys_memflag(_psdata_loc, 1, MEMFLAG_PRESENT);
	_psdata_loc->base = __executable_start;
	/* sets ->desc */
	progname = shortname(ed->argv[0]);
	setprogname(progname);

	_klogf("_start2 %s %p", progname, __executable_start);

	_sys_intr_set(intr_trampoline);
	intr_set(intr_default);
	__setinitialcwd(ed->cwd);

	exit(main(ed->argc, ed->argv, ed->envp));
}
