#include <shared/printf.h>
#include <user/lib/elf.h>
#include <user/lib/syscall.c>

const char *str = "Hello!\n", *str2 = "World.\n";

__attribute__((visibility("hidden")))
extern struct Elf64_Dyn _DYNAMIC[];

__attribute__((visibility("hidden")))
extern char _image_base[];

static void printf_backend(void *arg, const char *buf, size_t len) {
	(void)arg;
	_syscall_write(1, buf, len, -1);
}

int printf(const char *fmt, ...) {
	int ret = 0;
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, printf_backend, 0);
	va_end(argp);
	return ret;
}


static struct Elf64_Dyn *dyn_gettag(Elf64_Xword tag) {
	for (size_t i = 0;; i++) {
		if (_DYNAMIC[i].d_tag == tag) return &_DYNAMIC[i];
		if (_DYNAMIC[i].d_tag == DT_NULL) return NULL;
	}
}

static void reloc(void) {
	// TODO DT_REL
	if (dyn_gettag(DT_PLTGOT) || dyn_gettag(DT_JMPREL)) {
		printf("elf: unimplemented tag in _DYNAMIC\n");
	}

	struct Elf64_Dyn *rela_tag = dyn_gettag(DT_RELA);
	if (rela_tag) {
		/* not checking pointer validity,
		 * crashing on an invalid elf is fine */
		size_t relasz = dyn_gettag(DT_RELASZ)->d_val;
		size_t relaent = dyn_gettag(DT_RELAENT)->d_val;
		for (size_t o = 0; o < relasz; o += relaent) {
			struct Elf64_Rela *r = (void*)_image_base + rela_tag->d_ptr + o;
			uintptr_t *target = (void*)_image_base + r->r_offset;

			switch (ELF64_R_TYPE(r->r_info)) {
				case R_X86_64_RELATIVE:
					*target = (uintptr_t)&_image_base + r->r_addend;
					break;
				default:
					printf("elf: unsupported relocation type\n");
			}
		}
	}
}

int main(void) {
	printf("loaded at %x\n", &_image_base);
	reloc();
	printf(str);
	printf(str2);
	_syscall_exit(0);
}
