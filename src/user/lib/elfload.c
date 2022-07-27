#include <camellia/execbuf.h>
#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/elf.h>
#include <user/lib/elfload.h>

void elf_execf(FILE *f) {
	const size_t cap = 0x60000;
	size_t pos = 0;
	void *buf = malloc(cap); // TODO a way to get file size
	fseek(f, 0, SEEK_SET);
	if (buf && fread(buf, 1, cap - pos, f))
		elf_exec(buf);
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

/* frees memory outside of [low; low + len] and jumps to *entry */
static void freejmp(void *entry, void *low, size_t len) {
	uintptr_t high = (uintptr_t)low + len;
	uint64_t buf[] = {
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, 0, (uintptr_t)low, 0, 0,
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, high, ~0 - 0xF000 - high, 0, 0,
		EXECBUF_JMP, (uintptr_t)entry,
	};
	_syscall_execbuf(buf, sizeof buf);
	// should never return
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

void elf_exec(void *base) {
	struct Elf64_Ehdr *ehdr = base;
	if (!valid_ehdr(ehdr)) return;

	void *exebase = elf_loadmem(ehdr);
	if (!exebase) return;

	freejmp(exebase + ehdr->e_entry, exebase, elf_spread(ehdr) + 0x1000);
}

void *elf_partialexec(void *base) {
	struct Elf64_Ehdr *ehdr = base;
	if (!valid_ehdr(ehdr)) return NULL;

	void *exebase = elf_loadmem(ehdr);
	if (!exebase) return NULL;

	return exebase + ehdr->e_entry;
}
