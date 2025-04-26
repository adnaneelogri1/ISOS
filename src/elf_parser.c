#include "elf_parser.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int read_elf_header(const char* filename, elf_header* hdr) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    
    if (read(fd, hdr, sizeof(elf_header)) != sizeof(elf_header)) {
        perror("read failed");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

int read_program_headers(int fd, elf_header* hdr, elf_phdr** phdrs) {
    if (fd < 0) {
        return -1;
    }
    
    if (hdr->e_phentsize != sizeof(elf_phdr)) {
        perror("Program header size mismatch");
        return -1;
    }
    
    *phdrs = (elf_phdr*)malloc(hdr->e_phnum * sizeof(elf_phdr));
    if (*phdrs == NULL) {
        perror("malloc failed");
        return -1;
    }
    
    if (lseek(fd, hdr->e_phoff, SEEK_SET) == -1) {
        perror("lseek failed");
        free(*phdrs);
        *phdrs = NULL;
        return -1;
    }
    
    size_t size = hdr->e_phnum * sizeof(elf_phdr);
    if (read(fd, *phdrs, size) != size) {
        perror("read program headers failed");
        free(*phdrs);
        *phdrs = NULL;
        return -1;
    }
    
    return 0;
}

int check_valid_lib(elf_header* hdr) {
    if (hdr->e_ident[0] != ELF_MAGIC0 || 
        hdr->e_ident[1] != ELF_MAGIC1 || 
        hdr->e_ident[2] != ELF_MAGIC2 || 
        hdr->e_ident[3] != ELF_MAGIC3) {
        printf("Not an ELF file\n");
        return -1;
    }
    
    if (hdr->e_ident[4] != ELFCLASS64) {
        printf("Not a 64-bit ELF\n");
        return -1;
    }
    
    if (hdr->e_type != ET_DYN) {
        printf("Not a shared library (expected ET_DYN type)\n");
        return -1;
    }
        
    if (hdr->e_ehsize != sizeof(elf_header)) {
        printf("Invalid ELF header size\n");
        return -1;
    }
    
    if (hdr->e_phnum <= 0) {
        printf("Invalid ELF file: no program headers found\n");
        return -1;
    }
    
    return 0;
}

void print_header(elf_header* hdr) {
    printf("ELF Header:\n");
    
    printf("  Magic:   ");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", hdr->e_ident[i]);
    }
    printf("\n");
    
    printf("  Class:                             ");
    switch (hdr->e_ident[4]) {
        case 1:  printf("ELF32\n"); break;
        case 2:  printf("ELF64\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_ident[4]); break;
    }
    
    printf("  Type:                              ");
    switch (hdr->e_type) {
        case 1:  printf("REL (Relocatable file)\n"); break;
        case 2:  printf("EXEC (Executable file)\n"); break;
        case 3:  printf("DYN (Shared object file)\n"); break;
        case 4:  printf("CORE (Core file)\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_type); break;
    }
    
    printf("  Machine:                           ");
    switch (hdr->e_machine) {
        case 62: printf("Advanced Micro Devices X86-64\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_machine); break;
    }
    
    printf("  Entry point address:               0x%lx\n", hdr->e_entry);
    printf("  Start of program headers:          %lu (bytes into file)\n", hdr->e_phoff);
    printf("  Start of section headers:          %lu (bytes into file)\n", hdr->e_shoff);
    printf("  Number of program headers:         %d\n", hdr->e_phnum);
    printf("  Number of section headers:         %d\n", hdr->e_shnum);
}

void print_phdr(elf_phdr* phdr, int idx) {
    printf("  Program Header #%d:\n", idx);
    
    printf("    Type:              ");
    switch (phdr->p_type) {
        case PT_LOAD:    printf("LOAD\n"); break;
        case PT_DYNAMIC: printf("DYNAMIC\n"); break;
        default:         printf("0x%x\n", phdr->p_type); break;
    }
    
    printf("    Flags:             %c%c%c\n",
           (phdr->p_flags & PF_R) ? 'R' : '-',
           (phdr->p_flags & PF_W) ? 'W' : '-',
           (phdr->p_flags & PF_X) ? 'X' : '-');
    
    printf("    Offset:            0x%lx\n", phdr->p_offset);
    printf("    Virtual Address:   0x%lx\n", phdr->p_vaddr);
    printf("    Physical Address:  0x%lx\n", phdr->p_paddr);
    printf("    File Size:         0x%lx (%lu bytes)\n", phdr->p_filesz, phdr->p_filesz);
    printf("    Memory Size:       0x%lx (%lu bytes)\n", phdr->p_memsz, phdr->p_memsz);
    printf("    Alignment:         0x%lx\n", phdr->p_align);
}