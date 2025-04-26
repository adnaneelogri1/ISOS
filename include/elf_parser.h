#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>

#define ELF_MAGIC0  0x7f
#define ELF_MAGIC1  'E'
#define ELF_MAGIC2  'L'
#define ELF_MAGIC3  'F'

#define ELFCLASS64  2

#define ET_DYN      3

#define PT_LOAD     1
#define PT_DYNAMIC  2

#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4

#define R_X86_64_RELATIVE 8
#define R_ACCH64_RELATIVE 1027

typedef struct {
    unsigned char   e_ident[16];
    uint16_t        e_type;
    uint16_t        e_machine;
    uint32_t        e_version;
    uint64_t        e_entry;
    uint64_t        e_phoff;
    uint64_t        e_shoff;
    uint32_t        e_flags;
    uint16_t        e_ehsize;
    uint16_t        e_phentsize;
    uint16_t        e_phnum;
    uint16_t        e_shentsize;
    uint16_t        e_shnum;
    uint16_t        e_shstrndx;
} elf_header;

typedef struct {
    uint32_t        p_type;
    uint32_t        p_flags;
    uint64_t        p_offset;
    uint64_t        p_vaddr;
    uint64_t        p_paddr;
    uint64_t        p_filesz;
    uint64_t        p_memsz;
    uint64_t        p_align;
} elf_phdr;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t  r_addend;
} Elf64_Rela;

typedef struct {
    uint32_t    st_name;
    uint8_t     st_info;
    uint8_t     st_other;
    uint16_t    st_shndx;
    uint64_t    st_value;
    uint64_t    st_size;
} Elf64_Sym;


int read_elf_header(const char* filename, elf_header* hdr);
int read_program_headers(int fd, elf_header* hdr, elf_phdr** phdrs);
int check_valid_lib(elf_header* hdr);
void print_header(elf_header* hdr);
void print_phdr(elf_phdr* phdr, int idx);

int load_library(int fd, elf_header* hdr, elf_phdr* phdrs, void** out_base_addr);
int perform_relocations(void* base_addr, elf_header* hdr, elf_phdr* phdrs);
int find_dynamic_symbol(void* base_addr, elf_header* hdr, elf_phdr* phdrs, 
                    const char* name, void** symbol_addr);
#endif