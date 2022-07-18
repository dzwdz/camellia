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
		&& h->e_type == ET_EXEC
		&& h->e_machine == EM_X86_64
		&& h->e_version == 1;
}

static bool load_phdr(const void *base, size_t idx) {
	const struct Elf64_Ehdr *ehdr = base;
	const struct Elf64_Phdr *phdr = base + ehdr->e_phoff + idx * ehdr->e_phentsize;

	if (phdr->p_type != PT_LOAD) {
		printf("unknown type %x\n", phdr->p_type);
		return false;
	}
	// TODO overlap check
	// TODO don't ignore flags
	_syscall_memflag((void*)phdr->p_vaddr, phdr->p_memsz, MEMFLAG_PRESENT);
	// TODO check that filesz <= memsz
	memcpy((void*)phdr->p_vaddr, base + phdr->p_offset, phdr->p_filesz);
	return true;
}

void elf_exec(void *base) {
	struct Elf64_Ehdr *ehdr = base;
	if (!valid_ehdr(ehdr)) return;
	for (size_t phi = 0; phi < ehdr->e_phnum; phi++) {
		if (!load_phdr(base, phi))
			return;
	}
	((void(*)())ehdr->e_entry)();
	_syscall_exit(1);
}
