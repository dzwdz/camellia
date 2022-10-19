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

_Noreturn void _start2(struct execdata *ed) {
	const char *progname;
	elf_selfreloc();
	__setinitialcwd(ed->cwd);

	progname = shortname(ed->argv[0]);
	setprogname(progname);
	_klogf("_start2 %s 0x%x", progname, _image_base);

	exit(main(ed->argc, ed->argv, ed->envp));
}
