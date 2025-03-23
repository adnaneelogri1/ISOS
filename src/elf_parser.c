#include "../include/elf_parser.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int read_elf_header(const char* filename, elf_header* hdr) {
    // Open file for reading
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    
    // Read the header
    if (read(fd, hdr, sizeof(elf_header)) != sizeof(elf_header)) {
        perror("read failed");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

int check_valid_lib(elf_header* hdr) {
    // Check magic number
    if (hdr->e_ident[0] != ELF_MAGIC0 || 
        hdr->e_ident[1] != ELF_MAGIC1 || 
        hdr->e_ident[2] != ELF_MAGIC2 || 
        hdr->e_ident[3] != ELF_MAGIC3) {
        printf("Not an ELF file\n");
        return -1;
    }
    
    // Check if 64-bit
    if (hdr->e_ident[4] != ELFCLASS64) {
        printf("Not a 64-bit ELF\n");
        return -1;
    }
    
    // Check if shared library
    if (hdr->e_type != ET_DYN) {
        printf("Not a shared library (expected ET_DYN type)\n");
        return -1;
    }
        
    // Verify header size matches our structure size
    if (hdr->e_ehsize != sizeof(elf_header)) {
        printf("Invalid ELF header size\n");
        return -1;
    }
    
    // Make sure there is at least one program header
    if (hdr->e_phnum <= 0) {
        printf("Invalid ELF file: no program headers found\n");
        return -1;
    }
    
    return 0;
}

void print_header(elf_header* hdr) {
    printf("ELF Header:\n");
    
    // Magic bytes - print all 16 bytes of e_ident
    printf("  Magic:   ");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", hdr->e_ident[i]);
    }
    printf("\n");
    
    // Class
    printf("  Class:                             ");
    switch (hdr->e_ident[4]) {
        case 1:  printf("ELF32\n"); break;
        case 2:  printf("ELF64\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_ident[4]); break;
    }
    
    // Data encoding
    printf("  Data:                              ");
    switch (hdr->e_ident[5]) {
        case 1:  printf("2's complement, little endian\n"); break;
        case 2:  printf("2's complement, big endian\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_ident[5]); break;
    }
    
    // Version
    printf("  Version:                           %d", hdr->e_ident[6]);
    if (hdr->e_ident[6] == 1) printf(" (current)");
    printf("\n");
    
    // OS/ABI
    printf("  OS/ABI:                            ");
    switch (hdr->e_ident[7]) {
        case 0:  printf("UNIX - System V\n"); break;
        case 3:  printf("UNIX - GNU\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_ident[7]); break;
    }
    
    // ABI Version
    printf("  ABI Version:                       %d\n", hdr->e_ident[8]);
    
    // Type
    printf("  Type:                              ");
    switch (hdr->e_type) {
        case 1:  printf("REL (Relocatable file)\n"); break;
        case 2:  printf("EXEC (Executable file)\n"); break;
        case 3:  printf("DYN (Shared object file)\n"); break;
        case 4:  printf("CORE (Core file)\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_type); break;
    }
    
    // Machine
    printf("  Machine:                           ");
    switch (hdr->e_machine) {
        case 62: printf("Advanced Micro Devices X86-64\n"); break;
        default: printf("Unknown (%d)\n", hdr->e_machine); break;
    }
    
    // Version
    printf("  Version:                           0x%x\n", hdr->e_version);
    
    // Entry point
    printf("  Entry point address:               0x%lx\n", hdr->e_entry);
    
    // Program headers start
    printf("  Start of program headers:          %lu (bytes into file)\n", hdr->e_phoff);
    
    // Section headers start
    printf("  Start of section headers:          %lu (bytes into file)\n", hdr->e_shoff);
    
    // Flags
    printf("  Flags:                             0x%x\n", hdr->e_flags);
    
    // Header size
    printf("  Size of this header:               %d (bytes)\n", hdr->e_ehsize);
    
    // Program header size
    printf("  Size of program headers:           %d (bytes)\n", hdr->e_phentsize);
    
    // Number of program headers
    printf("  Number of program headers:         %d\n", hdr->e_phnum);
    
    // Section header size
    printf("  Size of section headers:           %d (bytes)\n", hdr->e_shentsize);
    
    // Number of section headers
    printf("  Number of section headers:         %d\n", hdr->e_shnum);
    
    // Section header string table index
    printf("  Section header string table index: %d\n", hdr->e_shstrndx);
}