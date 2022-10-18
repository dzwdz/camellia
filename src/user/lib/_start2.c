#include <camellia/syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <user/lib/elfload.h>

int main(int argc, char **argv, char **envp);

_Noreturn void _start2(struct execdata *ed) {
	elf_selfreloc();
	__setinitialcwd(ed->cwd);
	exit(main(ed->argc, ed->argv, ed->envp));
}
