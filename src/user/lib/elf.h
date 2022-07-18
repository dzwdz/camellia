/* shamelessly stolen from https://github.com/adachristine/sophia/tree/main/api/elf */
#pragma once
#include <stdint.h>

typedef uint8_t Elf_Byte;

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EI_VERSION 6
#define EV_NONE 0
#define EV_CURRENT 1

#define ELFOSABI_NONE 0
#define ELFOSABI_SYSV EI_OSABI_NONE

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOOS 0xfe00
#define ET_HIOS 0xfeff

#define EM_NONE 0

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2

#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x04

#define SHT_NULL 0
#define SHT_PROGITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXEC  0x4
#define SHF_MERGE 0x10
#define SHF_STRINGS 0x20

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_JMPREL 0x17

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Section;
typedef uint16_t Elf64_Versym;
typedef uint16_t Elf64_Half;
typedef int32_t Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef int64_t Elf64_Sxword;
typedef uint64_t Elf64_Xword;

typedef struct Elf64_Ehdr Elf64_Ehdr;
typedef struct Elf64_Phdr Elf64_Phdr;
typedef struct Elf64_Shdr Elf64_Shdr;
typedef struct Elf64_Sym Elf64_Sym;
typedef struct Elf64_Dyn Elf64_Dyn;

typedef struct Elf64_Rel Elf64_Rel;
typedef struct Elf64_Rela Elf64_Rela;

#define EM_X86_64 62

struct Elf64_Ehdr
{
    Elf_Byte e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

struct Elf64_Phdr
{
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

struct Elf64_Shdr
{
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Off sh_addralign;
    Elf64_Xword sh_entsize;
};

struct Elf64_Sym
{
    Elf64_Word st_name;
    Elf_Byte st_info;
    Elf_Byte st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
};

struct Elf64_Dyn
{
    Elf64_Xword d_tag;
    union
    {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    };
};

#define ELF64_R_SYM(info) ((info)>>32)
#define ELF64_R_TYPE(info) ((Elf64_Word)(info))
#define ELF64_R_INFO(sym, type) (((Elf64_Xword)(sym)<<32)+(Elf64_Xword)(type))

#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE 8

struct Elf64_Rel
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
};

struct Elf64_Rela
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
};
