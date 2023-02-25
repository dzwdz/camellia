#include <stdio.h>
#include "elf.h"

__attribute__((visibility("hidden")))
extern struct Elf64_Dyn _DYNAMIC[];

__attribute__((visibility("hidden")))
extern char __executable_start[];

static struct Elf64_Dyn *dyn_gettag(Elf64_Xword tag) {
	for (size_t i = 0;; i++) {
		if (_DYNAMIC[i].d_tag == tag) return &_DYNAMIC[i];
		if (_DYNAMIC[i].d_tag == DT_NULL) return NULL;
	}
}

void elf_selfreloc(void) {
	// TODO DT_REL, DT_JMPREL

	struct Elf64_Dyn *rela_tag = dyn_gettag(DT_RELA);
	if (rela_tag) {
		/* not checking pointer validity,
		 * crashing on an invalid elf is fine */
		size_t relasz = dyn_gettag(DT_RELASZ)->d_val;
		size_t relaent = dyn_gettag(DT_RELAENT)->d_val;
		for (size_t o = 0; o < relasz; o += relaent) {
			struct Elf64_Rela *r = (void*)__executable_start + rela_tag->d_ptr + o;
			uintptr_t *target = (void*)__executable_start + r->r_offset;

			switch (ELF64_R_TYPE(r->r_info)) {
				case R_X86_64_RELATIVE:
					*target = (uintptr_t)&__executable_start + r->r_addend;
					break;
				default:
					_klogf("elf: unsupported relocation type\n");
			}
		}
	}
}
