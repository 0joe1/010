#ifndef USERPROG_EXEC_H
#define USERPROG_EXEC_H
#include "stdint.h"

#define NIDENT (16)

typedef uint_32 Elf_Word;
typedef uint_32 Elf_Addr;
typedef uint_32 Elf_Off;

typedef uint_16 Elf_Half;

enum seg_type {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
};

typedef struct
{
    uint_8 e_ident[NIDENT];
    Elf_Half e_type;
    Elf_Half e_machine;
    Elf_Word e_version;
    Elf_Addr e_entry;
    Elf_Off e_phoff;
    Elf_Off e_shoff;
    Elf_Word e_flags;
    Elf_Half e_ehsize;
    Elf_Half e_phentsize;
    Elf_Half e_phnum;
    Elf_Half e_shentsize;
    Elf_Half e_shnum;
    Elf_Half e_shstrndx;
} Elf_Ehdr;

typedef struct
{
    Elf_Word p_type;
    Elf_Off p_offset;
    Elf_Addr p_vaddr;
    Elf_Addr p_paddr;
    Elf_Word p_filesz;
    Elf_Word p_memsz;
    Elf_Word p_flags;
    Elf_Word p_align;
} Elf_Phdr;

static Bool load_phdr(uint_32 fd,Elf_Phdr* phdr);
static int_32 load(const char* pathname);
int_32 sys_execv(const char* pathname,char** argv);

#endif
