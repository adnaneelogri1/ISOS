#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>

// ELF magic number
#define ELF_MAGIC0  0x7f
#define ELF_MAGIC1  'E'
#define ELF_MAGIC2  'L'
#define ELF_MAGIC3  'F'

// ELF class types
#define ELFCLASS64  2  // 64-bit

// ELF file types
#define ET_DYN      3  // shared library

// Program header types
#define PT_LOAD     1  // Loadable segment
#define PT_DYNAMIC  2  // Dynamic linking info

// Program header flags
#define PF_X        0x1  // Executable
#define PF_W        0x2  // Writable
#define PF_R        0x4  // Readable


// ELF header struct (64-bit) - must be exactly 64 bytes
typedef struct {
    unsigned char   e_ident[16];    // Magic bytes and other info
    uint16_t        e_type;         // Object type (exec, shared lib, etc)
    uint16_t        e_machine;      // Architecture
    uint32_t        e_version;      // ELF version
    uint64_t        e_entry;        // Entry point
    uint64_t        e_phoff;        // Program header offset
    uint64_t        e_shoff;        // Section header offset
    uint32_t        e_flags;        // Various flags
    uint16_t        e_ehsize;       // Header size
    uint16_t        e_phentsize;    // Program header entry size
    uint16_t        e_phnum;        // Number of program headers
    uint16_t        e_shentsize;    // Section header entry size
    uint16_t        e_shnum;        // Number of section headers
    uint16_t        e_shstrndx;     // Section header string table index
} elf_header;

// Program header struct (64-bit) - make sure size matches e_phentsize
typedef struct {
    uint32_t        p_type;         // Segment type (PT_LOAD, etc)
    uint32_t        p_flags;        // Segment flags (PF_X, PF_W, PF_R)
    uint64_t        p_offset;       // Offset in file
    uint64_t        p_vaddr;        // Virtual address
    uint64_t        p_paddr;        // Physical address (unused)
    uint64_t        p_filesz;       // Size in file
    uint64_t        p_memsz;        // Size in memory
    uint64_t        p_align;        // Alignment
} elf_phdr;

// Read and parse ELF header from a file
int read_elf_header(const char* filename, elf_header* hdr);

// Read program headers from a file
int read_program_headers(int fd, elf_header* hdr, elf_phdr** phdrs);

// Check if this is a valid shared library
int check_valid_lib(elf_header* hdr);

// Print header info (for debugging)
void print_header(elf_header* hdr);

// Print program header info (for debugging)
void print_phdr(elf_phdr* phdr, int idx);

int  load_library(int fd, elf_header* hdr, elf_phdr* phdrs) ;
#endif