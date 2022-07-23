#include <shared/execbuf.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <user/lib/elf.h>
#include <user/lib/elfload.h>

void elf_execf(libc_file *f) {
	const size_t cap = 0x60000;
	size_t pos = 0;
	void *buf = malloc(cap); // TODO a way to get file size
	if (!buf) goto fail;

	while (!f->eof) {
		long ret = file_read(f, buf, cap - pos);
		if (ret < 0) goto fail;
		pos += ret;
		if (pos >= cap) goto fail;
	}
	elf_exec(buf);

fail:
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

/* frees memory outside of [low; high] and jumps to *entry */
static void freejmp(void *entry, void *low, void *high) {
	uint64_t buf[] = {
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, 0, (uintptr_t)low, 0, 0,
		EXECBUF_SYSCALL, _SYSCALL_MEMFLAG, (uintptr_t)high, ~0 - 0xF000 - (uintptr_t)high, 0, 0,
		EXECBUF_JMP, (uintptr_t)entry,
	};
	_syscall_execbuf(buf, sizeof buf);
}

void elf_exec(void *base) {
	struct Elf64_Ehdr *ehdr = base;
	void *exebase;
	if (!valid_ehdr(ehdr)) return;
	_syscall_debug_klog("here", 4);
	size_t spread = elf_spread(base);
	switch (ehdr->e_type) {
		case ET_EXEC:
			exebase = (void*)0;
			break;
		case ET_DYN:
			exebase = _syscall_memflag((void*)0x1000, spread, MEMFLAG_FINDFREE);
			if (!exebase) {
				printf("elf: out of memory\n");
				_syscall_exit(1);
			}
			break;
		default:
			return;
	}
	for (size_t phi = 0; phi < ehdr->e_phnum; phi++) {
		if (!load_phdr(base, exebase, phi))
			return;
	}

	freejmp(exebase + ehdr->e_entry, exebase, exebase + spread + 0x1000);
	printf("elf: execbuf failed?");
}
