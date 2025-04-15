#include "../include/elf_parser.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

// Define relocation-related constants and structures
#define R_X86_64_RELATIVE 8

typedef struct {
    uint64_t r_offset;  // Location to apply relocation
    uint64_t r_info;    // Symbol index and type
    int64_t  r_addend;  // Constant addend
} Elf64_Rela;

// Find dynamic segment and perform relocations
int perform_relocations(void* base_addr, elf_header* hdr, elf_phdr* phdrs) {
    fprintf(stderr, "Starting relocations...\n");

    // Find the dynamic segment
    elf_phdr* dyn_segment = NULL;
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_segment = &phdrs[i];
            fprintf(stderr, "Found PT_DYNAMIC segment at index %d\n", i);
            break;
        }
    }
    
    if (!dyn_segment) {
        fprintf(stderr, "No dynamic segment found, no relocations needed\n");
        return 0;
    }
    
    // Variables to hold relocation table info
    Elf64_Rela* rela = NULL;
    int rela_count = 0;
    
    // Process dynamic entries to find relocation table
    uint64_t* dynamic = (uint64_t*)((char*)base_addr + dyn_segment->p_vaddr);
    fprintf(stderr, "Dynamic section at address %p\n", dynamic);
    
    int i = 0;
    
    // Parse dynamic section entries
    while (dynamic[i] != 0) {
        uint64_t tag = dynamic[i++];
        uint64_t val = dynamic[i++];
        
        fprintf(stderr, "Dynamic entry: tag=0x%lx, val=0x%lx\n", tag, val);
        
        // DT_RELA = 7, points to relocation table
        if (tag == 7) {
            rela = (Elf64_Rela*)((char*)base_addr + val);
            fprintf(stderr, "Found RELA table at %p\n", rela);
        }
        // DT_RELASZ = 8, size of relocation table
        else if (tag == 8) {
            rela_count = val / sizeof(Elf64_Rela);
            fprintf(stderr, "RELA table contains %d entries\n", rela_count);
        }
    }
    
    if (!rela || rela_count == 0) {
        fprintf(stderr, "No relocations found\n");
        return 0;
    }
    
    fprintf(stderr, "Processing %d relocations...\n", rela_count);
    
    // Process each relocation entry
    for (int i = 0; i < rela_count; i++) {
        // Extract relocation info
        uint64_t offset = rela[i].r_offset;
        uint32_t type = rela[i].r_info & 0xffffffff;
        
        fprintf(stderr, "Relocation %d: offset=0x%lx, type=%u\n", i, offset, type);
        
        // Check if it's a RELATIVE relocation
        if (type == R_X86_64_RELATIVE) {
            // Address to be relocated
            uint64_t* target = (uint64_t*)((char*)base_addr + offset);
            
            fprintf(stderr, "  R_X86_64_RELATIVE: target=%p, old value=0x%lx\n", target, *target);
            
            // Perform the relocation - adjust address by base address
            uint64_t new_value = (uint64_t)base_addr + rela[i].r_addend;
            *target = new_value;
            
            fprintf(stderr, "  Updated to 0x%lx\n", new_value);
        }
    }
    
    fprintf(stderr, "Relocations completed successfully\n");
    return 0;
}
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

int read_program_headers(int fd, elf_header* hdr, elf_phdr** phdrs) {
    // Check if file is valid
    if (fd < 0) {
        return -1;
    }
    
    // Verify program header size matches our structure
    if (hdr->e_phentsize != sizeof(elf_phdr)) {
        perror("Program header size mismatch: expected , got \n");
        return -1;
    }
    
    // Allocate memory for program headers
    *phdrs = (elf_phdr*)malloc(hdr->e_phnum * sizeof(elf_phdr));
    if (*phdrs == NULL) {
        perror("malloc failed");
        return -1;
    }
    
    // Seek to program header table
    if (lseek(fd, hdr->e_phoff, SEEK_SET) == -1) {
        perror("lseek failed");
        free(*phdrs);
        *phdrs = NULL;
        return -1;
    }
    
    // Read all program headers
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
int load_library(int fd, elf_header* hdr, elf_phdr* phdrs) {
    size_t page_size = getpagesize();
    
    // 1. Find boundaries of all PT_LOAD segments
    uint64_t first_vaddr = UINT64_MAX;
    uint64_t last_vaddr_end = 0;
    int load_segments = 0;
    
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            load_segments++;
            
            if (phdrs[i].p_vaddr < first_vaddr)
                first_vaddr = phdrs[i].p_vaddr;
            
            uint64_t seg_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            if (seg_end > last_vaddr_end)
                last_vaddr_end = seg_end;
        }
    }
    
    if (load_segments == 0) {
        fprintf(stderr, "ERROR: No PT_LOAD segments found\n");
        return -1;
    }
    
    // Align to page boundary
    uint64_t base_offset = first_vaddr & ~(page_size - 1);
    
    // Calculate total size needed with proper alignment
    size_t total_size = last_vaddr_end - base_offset;
    total_size = (total_size + page_size - 1) & ~(page_size - 1);
    
    // 2. Create initial mapping
    void* base_addr = mmap(
        NULL,                   // Let kernel choose the address
        total_size,             // Total size of segments
        PROT_READ | PROT_WRITE, // Initial RW permissions for easier setup
        MAP_PRIVATE | MAP_ANONYMOUS, // Private mapping
        -1,                     // No file descriptor
        0                       // No offset
    );
    
    if (base_addr == MAP_FAILED) {
        perror("Initial mmap failed");
        return -1;
    }
    
    // Clear the entire region to avoid data leaks
    memset(base_addr, 0, total_size);
    
    // Calculate load base address
    uint64_t base_address = (uint64_t)base_addr - base_offset;
    
    // 3. Map each PT_LOAD segment
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            // Calculate addresses
            uint64_t seg_vaddr = phdrs[i].p_vaddr;
            uint64_t aligned_vaddr = seg_vaddr & ~(page_size - 1);
            uint64_t offset_in_page = seg_vaddr - aligned_vaddr;
            
            // Calculate load address
            void* load_addr = (void*)(base_address + aligned_vaddr);
            
            // Verify alignment
            if ((uint64_t)load_addr % page_size != 0) {
                fprintf(stderr, "ERROR: Load address not page-aligned: %p\n", load_addr);
                munmap(base_addr, total_size);
                return -1;
            }
            
            // Calculate size
            size_t file_size = phdrs[i].p_filesz;
            size_t mem_size = phdrs[i].p_memsz;
            
            // Calculate source offset in file
            off_t file_offset = phdrs[i].p_offset;
            
            // Make sure we don't read beyond file end
            if (lseek(fd, 0, SEEK_END) < file_offset + file_size) {
                fprintf(stderr, "ERROR: Segment extends beyond file size\n");
                munmap(base_addr, total_size);
                return -1;
            }
            
            // Reset file pointer
            lseek(fd, 0, SEEK_SET);
            
            // Load the segment data
            if (file_size > 0) {
                // Target address for data
                void* target = (void*)((char*)load_addr + offset_in_page);
                
                // Seek to the correct position
                if (lseek(fd, file_offset, SEEK_SET) != file_offset) {
                    perror("lseek failed");
                    munmap(base_addr, total_size);
                    return -1;
                }
                
                // Read data directly with read()
                ssize_t bytes_read = read(fd, target, file_size);
                if (bytes_read != file_size) {
                    perror("read failed");
                    munmap(base_addr, total_size);
                    return -1;
                }
            }
            
            // Initialize BSS section if needed
            if (mem_size > file_size) {
                void* bss_start = (void*)((char*)load_addr + offset_in_page + file_size);
                size_t bss_size = mem_size - file_size;
                
                // Use standard memset for BSS
                memset(bss_start, 0, bss_size);
            }
            
        }
    }
    if (perform_relocations((void*)base_address, hdr, phdrs) != 0) {
    fprintf(stderr, "Failed to perform relocations\n");
    munmap(base_addr, total_size);
    return -1;
}
    // 4. Apply correct protections to each segment
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            // Calculate addresses
            uint64_t seg_vaddr = phdrs[i].p_vaddr;
            uint64_t aligned_vaddr = seg_vaddr & ~(page_size - 1);
            uint64_t offset_in_page = seg_vaddr - aligned_vaddr;
            void* load_addr = (void*)(base_address + aligned_vaddr);
            
            // Calculate aligned size
            size_t raw_size = phdrs[i].p_memsz + offset_in_page;
            size_t aligned_size = (raw_size + page_size - 1) & ~(page_size - 1);
            
            // Convert ELF flags to protection flags
            int prot = 0;
            if (phdrs[i].p_flags & PF_R) prot |= PROT_READ;
            if (phdrs[i].p_flags & PF_W) prot |= PROT_WRITE;
            if (phdrs[i].p_flags & PF_X) prot |= PROT_EXEC;
            
            // Apply protections
            if (mprotect(load_addr, aligned_size, prot) != 0) {
                perror("mprotect failed");
                munmap(base_addr, total_size);
                return -1;
            }
        }
    }
    
    fprintf(stderr, "DEBUG: Library successfully loaded at base: %p\n", (void*)base_address);
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

void print_phdr(elf_phdr* phdr, int idx) {
    printf("  Program Header #%d:\n", idx);
    
    // Print segment type
    printf("    Type:              ");
    switch (phdr->p_type) {
        case PT_LOAD:    printf("LOAD\n"); break;
        case PT_DYNAMIC: printf("DYNAMIC\n"); break;
        default:         printf("0x%x\n", phdr->p_type); break;
    }
    
    // Print flags
    printf("    Flags:             %c%c%c\n",
           (phdr->p_flags & PF_R) ? 'R' : '-',
           (phdr->p_flags & PF_W) ? 'W' : '-',
           (phdr->p_flags & PF_X) ? 'X' : '-');
    
    // Print offset
    printf("    Offset:            0x%lx\n", phdr->p_offset);
    
    // Print virtual address
    printf("    Virtual Address:   0x%lx\n", phdr->p_vaddr);
    
    // Print physical address
    printf("    Physical Address:  0x%lx\n", phdr->p_paddr);
    
    // Print sizes
    printf("    File Size:         0x%lx (%lu bytes)\n", phdr->p_filesz, phdr->p_filesz);
    printf("    Memory Size:       0x%lx (%lu bytes)\n", phdr->p_memsz, phdr->p_memsz);
    
    // Print alignment
    printf("    Alignment:         0x%lx\n", phdr->p_align);
}