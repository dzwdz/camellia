#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/elfload.h>

int main(int argc, char **argv, char **envp);

__attribute__((visibility("hidden")))
extern char _image_base[];

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
	_syscall_intr_set(intr_trampoline);
	intr_set(intr_default);
	__setinitialcwd(ed->cwd);

	progname = shortname(ed->argv[0]);
	setprogname(progname);
	_klogf("_start2 %s %p", progname, _image_base);

	exit(main(ed->argc, ed->argv, ed->envp));
}
