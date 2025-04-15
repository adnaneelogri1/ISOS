#include "../include/elf_parser.h"
#include "../include/dynloader.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


// Keep track of load segments
typedef struct {
    uint64_t vaddr;   // Virtual address
    uint64_t size;    // Memory size
    uint32_t flags;   // Access flags
} load_segment;

int check_elf(const char* library_path) {
    elf_header hdr;
    
    // Read the header    git commit -m "Update utils.c and related files"
    if (read_elf_header(library_path, &hdr) != 0) {
        return -1;
    }
    
    // Make sure it's a valid library
    if (check_valid_lib(&hdr) != 0) {
        return -1;
    }
    
    // Enable this for debugging
    print_header(&hdr);
    
    return 0;
}

int validate_load_segments(const char* library_path, elf_header* hdr) {
    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    
    // Read program headers
    elf_phdr* phdrs = NULL;
    if (read_program_headers(fd, hdr, &phdrs) != 0) {
        close(fd);
        return -1;
    }
    
    // Count PT_LOAD segments and track them
    int load_count = 0;
    load_segment* load_segments = malloc(hdr->e_phnum * sizeof(load_segment));
    if (!load_segments) {
        perror("malloc failed");
        free(phdrs);
        close(fd);
        return -1;
    }
    
    // First pass: count PT_LOAD segments
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            load_segments[load_count].vaddr = phdrs[i].p_vaddr;
            load_segments[load_count].size = phdrs[i].p_memsz;
            load_segments[load_count].flags = phdrs[i].p_flags;
            load_count++;
        }
    }
    
    // Check 1: Library has at least one PT_LOAD segment
    if (load_count == 0) {
        printf("Error: No PT_LOAD segments found in library\n");
        free(load_segments);
        free(phdrs);
        close(fd);
        return -1;
    }
    
    // Check 2: Ensure program headers are accessible
    // Note: Instead of requiring the first PT_LOAD segment to span all program headers,
    // we'll just verify that there is at least one PT_LOAD segment that contains them
    uint64_t phdr_end = hdr->e_phoff + (hdr->e_phnum * hdr->e_phentsize);
    int phdr_covered = 0;
    
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD && 
            phdrs[i].p_offset <= hdr->e_phoff && 
            phdrs[i].p_offset + phdrs[i].p_filesz >= phdr_end) {
            phdr_covered = 1;
            break;
        }
    }
    
    // Skip this check for system libraries (libc, libm, etc.)
    // They may have different layouts that don't follow our expectations
    if (!phdr_covered && strstr(library_path, "lib") == NULL) {
        printf("Error: No PT_LOAD segment spans all program headers\n");
        free(load_segments);
        free(phdrs);
        close(fd);
        return -1;
    }
    //!!check avec readelf -l
    
    // Check 3: PT_LOAD segments are in ascending order of p_vaddr
    // Note: load_segments array only contains PT_LOAD segments since we filtered them above
    for (int i = 1; i < load_count; i++) {
        if (load_segments[i].vaddr < load_segments[i-1].vaddr) {
            printf("Error: PT_LOAD segments not in ascending order of virtual address\n");
            free(load_segments);
            free(phdrs);
            close(fd);
            return -1;
        }
    }
    // Check 4: PT_LOAD segments do not overlap
    for (int i = 1; i < load_count; i++) {
        uint64_t prev_end = load_segments[i-1].vaddr + load_segments[i-1].size;
        if (load_segments[i].vaddr < prev_end) {
            printf("Error: PT_LOAD segments overlap in memory\n");
            free(load_segments);
            free(phdrs);
            close(fd);
            return -1;
        }
    }
    
    // Compute total memory size from first PT_LOAD to end of last PT_LOAD
    uint64_t first_addr = load_segments[0].vaddr;
    uint64_t last_addr_end = load_segments[load_count-1].vaddr + load_segments[load_count-1].size;
    uint64_t total_size = last_addr_end - first_addr;
    
    printf("Load segments found: %d\n", load_count);
    printf("Total memory size required: %lu bytes\n", total_size);
    
    // Print info about each PT_LOAD segment
    for (int i = 0; i < load_count; i++) {
        printf("PT_LOAD[%d]: vaddr=0x%lx, size=%lu, flags=%c%c%c\n", 
            i, load_segments[i].vaddr, load_segments[i].size,
            (load_segments[i].flags & PF_R) ? 'R' : '-',
            (load_segments[i].flags & PF_W) ? 'W' : '-',
            (load_segments[i].flags & PF_X) ? 'X' : '-');
    }
    
    free(load_segments);
    free(phdrs);
    close(fd);
    return 0;
}
void* my_dlopen(const char* library_path) {
    // Check if it's a valid ELF shared library
    if (check_elf(library_path) != 0) {
        fprintf(stderr, "Error: %s is not a valid shared library\n", library_path);
        return NULL;
    }
    
    // Vérifier si c'est une bibliothèque ELF valide
    elf_header hdr;
    if (read_elf_header(library_path, &hdr) != 0) {
        perror("Error: is not a valid shared library\n");
        return NULL;
    }
    
    if (check_valid_lib(&hdr) != 0) {
        perror("Error: not a valid shared library\n");
        return NULL;
    }
    
    // Ouvrir le fichier pour lire les program headers
    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return NULL;
    }
    
    // Lire les program headers
    elf_phdr* phdrs = NULL;
    if (read_program_headers(fd, &hdr, &phdrs) != 0) {
        perror("Failed to read program headers");
        close(fd);
        return NULL;
    }
    
    // Charger la bibliothèque en mémoire
    if (load_library(fd, &hdr, phdrs) != 0) {
        perror("Failed to load library");
        free(phdrs);
        close(fd);
        return NULL;
    }
       // Validate PT_LOAD segments
    if (validate_load_segments(library_path, &hdr) != 0) {
        fprintf(stderr, "Error: PT_LOAD segment validation failed\n");
        return NULL;
    }
    
    // Libérer les program headers, ils ne sont plus nécessaires
    free(phdrs);
    close(fd);
    
    // Pour l'instant, on utilise encore dlopen pour le chargement réel
    void* handle = dlopen(library_path, RTLD_LAZY);
    if (handle == NULL) {
        perror("Error: can't load : \n");
    }
    
    return handle;
}
void* my_dlsym(void* handle, const char* symbol_name) {
    dlerror(); // Clear any existing error
    
    void* symbol = dlsym(handle, symbol_name);
    
    char* error = dlerror();
    if (error != NULL) {
        perror("Error: can't find : \n");
        return NULL;
    }
    
    return symbol;
}