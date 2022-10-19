#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/elf.h>
#include <user/lib/elfload.h>

void elf_execf(FILE *f, char **argv, char **envp) {
	void *buf;
	long buflen;

	fseek(f, 0, SEEK_END);
	buflen = ftell(f);
	if (buflen < 0) return; /* errno set by fseek */
	buf = malloc(buflen);

	// TODO don't read the entire file into memory
	fseek(f, 0, SEEK_SET);
	if (buf && fread(buf, 1, buflen, f))
		elf_exec(buf, argv, envp);
	free(buf);
}

static bool valid_ehdr(const struct Elf64_Ehdr *h) {
	return h->e_ident[0] == 0x7f
		&& h->e_ident[1] == 'E'
		&& h->e_ident[2] == 'L'
		&& h->e_ident[3] == 'F'
		&& h->e_machine == EM_X86_64
		&& h->e_version == 1;
}

static bool load_phdr(const void *elf, void *exebase, size_t idx) {
	const struct Elf64_Ehdr *ehdr = elf;
	const struct Elf64_Phdr *phdr = elf + ehdr->e_phoff + idx * ehdr->e_phentsize;

	if (phdr->p_type == PT_DYNAMIC) return true;

	if (phdr->p_type != PT_LOAD) {
		printf("unknown type %x\n", phdr->p_type);
		return false;
	}
	// TODO overlap check
	// TODO don't ignore flags
	_syscall_memflag(exebase + phdr->p_vaddr, phdr->p_memsz, MEMFLAG_PRESENT);
	// TODO check that filesz <= memsz
	memcpy(exebase + phdr->p_vaddr, elf + phdr->p_offset, phdr->p_filesz);
	return true;
}

static size_t elf_spread(const void *elf) {
	const struct Elf64_Ehdr *ehdr = elf;
	uintptr_t high = 0, low = ~0;
	for (size_t phi = 0; phi < ehdr->e_phnum; phi++) {
		const struct Elf64_Phdr *phdr = elf + ehdr->e_phoff + phi * ehdr->e_phentsize;
		if (high < phdr->p_vaddr + phdr->p_memsz)
			high = phdr->p_vaddr + phdr->p_memsz;
		if (low > phdr->p_vaddr)
			low = phdr->p_vaddr;
	}
	return high - low;
}

static void *memdup(const void *orig, size_t len) {
	void *n = malloc(len);
	if (n) memcpy(n, orig, len);
	return n;
}

static const char *default_argv[] = {NULL};

/* frees memory outside of [low; low + len] and jumps to *entry
 * also sets up main's stack */
void _freejmp_chstack(void *entry, void *low, size_t len, const char **argv, char **envp, void *stack); // elfload.s
_Noreturn void execbuf_chstack(void *stack, void __user *buf, size_t len);
void _freejmp(void *entry, void *low, size_t imglen, const char **argv, char **envp) {
	void *stack = (void*)~0;
	struct execdata ed;
	size_t argv_len;

	if (!argv) argv = default_argv;

	ed.argc = 0;
	while (argv[ed.argc]) ed.argc++;
	argv_len = (ed.argc+1) * sizeof(char *);

	/* make a copy of argv, so it doesn't get overridden
	 * if it overlaps with the new stack. */
	argv = memdup(argv, argv_len);
	for (int i = 0; i < ed.argc; i++)
		argv[i] = strdup(argv[i]);

	stack -= argv_len;
	ed.argv = stack;

	for (int i = 0; i < ed.argc; i++) {
		size_t len = strlen(argv[i]) + 1;
		stack -= len;
		memcpy(stack, argv[i], len);
		ed.argv[i] = stack;
	}
	ed.argv[ed.argc] = NULL;

	/* push cwd */
	size_t len = absolutepath(NULL, NULL, 0);
	stack -= len;
	getcwd(stack, len);
	ed.cwd = stack;

	stack -= sizeof ed;
	memcpy(stack, &ed, sizeof ed);

	uintptr_t high = (uintptr_t)low + imglen;
	uint64_t buf[] = {
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, 0, (uintptr_t)low, 0, 0, 0,
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, high, ~0 - 0xF000 - high, 0, 0, 0,
		EXECBUF_JMP, (uintptr_t)entry,
	};
	execbuf_chstack(stack, buf, sizeof buf);
}

static void *elf_loadmem(struct Elf64_Ehdr *ehdr) {
	void *exebase;
	size_t spread = elf_spread(ehdr);
	switch (ehdr->e_type) {
		case ET_EXEC:
			exebase = (void*)0;
			break;
		case ET_DYN:
			exebase = _syscall_memflag((void*)0x1000, spread, MEMFLAG_FINDFREE);
			if (!exebase)
				return NULL;
			break;
		default:
			return NULL;
	}
	for (size_t phi = 0; phi < ehdr->e_phnum; phi++) {
		if (!load_phdr((void*)ehdr, exebase, phi)) {
			_syscall_memflag(exebase, spread, 0);
			return NULL;
		}
	}
	return exebase;
}

void elf_exec(void *base, char **argv, char **envp) {
	struct Elf64_Ehdr *ehdr = base;
	if (!valid_ehdr(ehdr)) return;

	void *exebase = elf_loadmem(ehdr);
	if (!exebase) return;
	_klogf("exebase 0x%x", exebase);

	void *newstack = _syscall_memflag((void*)0x1000, 0x1000, MEMFLAG_FINDFREE | MEMFLAG_PRESENT);
	if (!newstack) return;

	_freejmp_chstack(exebase + ehdr->e_entry, exebase, elf_spread(ehdr) + 0x1000, (const char**)argv, envp, newstack);
}

void *elf_partialexec(void *base) {
	struct Elf64_Ehdr *ehdr = base;
	if (!valid_ehdr(ehdr)) return NULL;

	void *exebase = elf_loadmem(ehdr);
	if (!exebase) return NULL;

	return exebase + ehdr->e_entry;
}
