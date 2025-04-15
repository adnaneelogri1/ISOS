#include "../include/elf_parser.h"
#include "../include/dynloader.h"
#include "../include/debug.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
} load_segment;

int check_elf(const char* library_path) {
    elf_header hdr;
    
    if (read_elf_header(library_path, &hdr) != 0) {
        return -1;
    }
    
    if (check_valid_lib(&hdr) != 0) {
        return -1;
    }
    
    print_header(&hdr);
    
    return 0;
}

int validate_load_segments(const char* library_path, elf_header* hdr) {
    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    
    elf_phdr* phdrs = NULL;
    if (read_program_headers(fd, hdr, &phdrs) != 0) {
        close(fd);
        return -1;
    }
    
    int load_count = 0;
    load_segment* load_segments = malloc(hdr->e_phnum * sizeof(load_segment));
    if (!load_segments) {
        perror("malloc failed");
        free(phdrs);
        close(fd);
        return -1;
    }
    
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            load_segments[load_count].vaddr = phdrs[i].p_vaddr;
            load_segments[load_count].size = phdrs[i].p_memsz;
            load_segments[load_count].flags = phdrs[i].p_flags;
            load_count++;
        }
    }
    
    if (load_count == 0) {
        printf("Error: No PT_LOAD segments found in library\n");
        free(load_segments);
        free(phdrs);
        close(fd);
        return -1;
    }
    
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
    
    if (!phdr_covered && strstr(library_path, "lib") == NULL) {
        printf("Error: No PT_LOAD segment spans all program headers\n");
        free(load_segments);
        free(phdrs);
        close(fd);
        return -1;
    }
    
    for (int i = 1; i < load_count; i++) {
        if (load_segments[i].vaddr < load_segments[i-1].vaddr) {
            printf("Error: PT_LOAD segments not in ascending order\n");
            free(load_segments);
            free(phdrs);
            close(fd);
            return -1;
        }
    }

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
    
    uint64_t first_addr = load_segments[0].vaddr;
    uint64_t last_addr_end = load_segments[load_count-1].vaddr + load_segments[load_count-1].size;
    uint64_t total_size = last_addr_end - first_addr;
    
    printf("Load segments found: %d\n", load_count);
    printf("Total memory size required: %lu bytes\n", total_size);
    
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
    if (check_elf(library_path) != 0) {
        fprintf(stderr, "Error: %s is not a valid shared library\n", library_path);
        return NULL;
    }
    
    elf_header hdr;
    if (read_elf_header(library_path, &hdr) != 0) {
        perror("Error reading header");
        return NULL;
    }
    
    if (check_valid_lib(&hdr) != 0) {
        perror("Error: not a valid library");
        return NULL;
    }
    
    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return NULL;
    }
    
    elf_phdr* phdrs = NULL;
    if (read_program_headers(fd, &hdr, &phdrs) != 0) {
        perror("Failed to read program headers");
        close(fd);
        return NULL;
    }
    
    if (load_library(fd, &hdr, phdrs) != 0) {
        perror("Failed to load library");
        free(phdrs);
        close(fd);
        return NULL;
    }
    
    if (validate_load_segments(library_path, &hdr) != 0) {
        fprintf(stderr, "Error: PT_LOAD segment validation failed\n");
        return NULL;
    }
    
    free(phdrs);
    close(fd);
    
    void* handle = dlopen(library_path, RTLD_LAZY);
    if (handle == NULL) {
        perror("Error loading library");
    }
    
    return handle;
}

void* my_dlsym(void* handle, const char* symbol_name) {
    // Adresse de base de notre bibliothèque chargée
    extern void* g_base_addr;
    extern elf_header g_hdr;
    extern elf_phdr* g_phdrs;
    
    if (!handle) {
        debug_error("Handle invalide");
        return NULL;
    }
    
    // D'abord essayer notre propre résolution de symboles
    void* symbol_addr = NULL;
    if (g_base_addr && g_phdrs) {
        if (find_dynamic_symbol(g_base_addr, &g_hdr, g_phdrs, symbol_name, &symbol_addr) == 0) {
            debug_detail("Symbole trouvé par notre résolveur");
            return symbol_addr;
        }
    }
    
    // Si notre résolution échoue, on utilise dlsym
    dlerror(); // Effacer toute erreur précédente
    
    void* symbol = dlsym(handle, symbol_name);
    
    char* error = dlerror();
    if (error != NULL) {
        debug_warn("dlsym a échoué");
        return NULL;
    }
    
    return symbol;
}