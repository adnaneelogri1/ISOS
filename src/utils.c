#include "debug.h"
#include "dynloader.h"
#include "elf_parser.h"
#include "isos-support.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void isos_trampoline();
asm(".pushsection .text,\"ax\",\"progbits\""
    "\n"
    "isos_trampoline:"
    "\n" POP_S(REG_ARG_1) "\n" POP_S(REG_ARG_2) "\n" PUSH_STACK_STATE "\n" CALL(
        loader_plt_resolver) "\n" POP_STACK_STATE
    "\n" JMP_REG(REG_RET) "\n"
    ".popsection"
    "\n");

/**
 * @param handler  : the loader handler returned by my_dlopen().
 * @param import_id: the identifier of the function to be called
 *                   from the imported symbol table.
 * @return the address of the function to be called by the trampoline.
 */
/**
 * @param handler  : the loader handler returned by my_dlopen().
 * @param import_id: the identifier of the function to be called
 *                   from the imported symbol table.
 * @return the address of the function to be called by the trampoline.
 */
typedef struct {
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
} load_segment;

int check_elf(const char *library_path) {
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

int validate_load_segments(const char *library_path, elf_header *hdr) {
    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    elf_phdr *phdrs = NULL;
    if (read_program_headers(fd, hdr, &phdrs) != 0) {
        close(fd);
        return -1;
    }

    int load_count = 0;
    load_segment *load_segments = malloc(hdr->e_phnum * sizeof(load_segment));
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
        if (phdrs[i].p_type == PT_LOAD && phdrs[i].p_offset <= hdr->e_phoff &&
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
        if (load_segments[i].vaddr < load_segments[i - 1].vaddr) {
            printf("Error: PT_LOAD segments not in ascending order\n");
            free(load_segments);
            free(phdrs);
            close(fd);
            return -1;
        }
    }

    for (int i = 1; i < load_count; i++) {
        uint64_t prev_end = load_segments[i - 1].vaddr + load_segments[i - 1].size;
        if (load_segments[i].vaddr < prev_end) {
            printf("Error: PT_LOAD segments overlap in memory\n");
            free(load_segments);
            free(phdrs);
            close(fd);
            return -1;
        }
    }

    uint64_t first_addr = load_segments[0].vaddr;
    uint64_t last_addr_end =
            load_segments[load_count - 1].vaddr + load_segments[load_count - 1].size;
    uint64_t total_size = last_addr_end - first_addr;

    printf("Load segments found: %d\n", load_count);
    printf("Total memory size required: %lu bytes\n", total_size);

    for (int i = 0; i < load_count; i++) {
        printf("PT_LOAD[%d]: vaddr=0x%lx, size=%lu, flags=%c%c%c\n", i,
               load_segments[i].vaddr, load_segments[i].size,
               (load_segments[i].flags & PF_R) ? 'R' : '-',
               (load_segments[i].flags & PF_W) ? 'W' : '-',
               (load_segments[i].flags & PF_X) ? 'X' : '-');
    }

    free(load_segments);
    free(phdrs);
    close(fd);
    return 0;
}

void *my_dlopen(const char *library_path) {
    if (check_elf(library_path) != 0) {
        debug_warn("Error: not a valid shared library");
        return NULL;
    }

    // Allocate handle structure
    lib_handle_t *handle = (lib_handle_t *) malloc(sizeof(lib_handle_t));
    if (!handle) {
        perror("Failed to allocate memory for handle");
        return NULL;
    }

    // Initialize handle
    explicit_bzero(handle, sizeof(lib_handle_t));
    
    // header
    elf_header hdr;
    if (read_elf_header(library_path, &hdr) != 0) {
        perror("Error reading header");
        free(handle);
        return NULL;
    }

    if (check_valid_lib(&hdr) != 0) {
        perror("Error: not a valid library");
        free(handle);
        return NULL;
    }

    int fd = open(library_path, O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        free(handle);
        return NULL;
    }
    
    // declare phdrs
    elf_phdr *phdrs = NULL;
    if (read_program_headers(fd, &hdr, &phdrs) != 0) {
        perror("Failed to read program headers");
        close(fd);
        free(handle);
        return NULL;
    }

    // Modified to store base_addr in the handle
    void *base_addr = NULL;
    
    // check variable hdr phdrs
    if (load_library(fd, &hdr, phdrs, &base_addr) != 0) {
        perror("Failed to load library");
        free(phdrs);
        close(fd);
        free(handle);
        return NULL;
    }
    handle->base_addr = base_addr;

    if (validate_load_segments(library_path, &hdr) != 0) {
        debug_warn("Error: PT_LOAD segment validation failed");
        free(phdrs);
        close(fd);
        free(handle);
        return NULL;
    }
    
    if (!(handle->base_addr && hdr.e_entry != 0)) {
        debug_warn("Error: Invalid entry point address");
        free(phdrs);
        close(fd);
        free(handle);
        return NULL;
    }

    // Fix alignment issues using uintptr_t for pointer arithmetic
    uintptr_t info_addr_int = (uintptr_t)base_addr + hdr.e_entry;
    
    // Check alignment before casting
    if (info_addr_int % sizeof(loader_info_t) != 0) {
        debug_error("Adresse de loader_info mal alignée");
        free(phdrs);
        close(fd);
        free(handle);  // Fix the memory leak here
        return NULL;
    }
    
    // Now it's safe to cast to loader_info_t*
    loader_info_t* info = (loader_info_t*)info_addr_int;
    
    // add tabsymbol to handle
    handle->imported_symbols = info->imported_symbols;
    handle->exported_symbols = info->exported_symbols;
    *(info->loader_handle) = handle;
    *(info->isos_trampoline) = &isos_trampoline;

    free(phdrs);  // Free the program headers as they're no longer needed
    close(fd);

    return (void *)handle;
}

void *my_dlsym(void *handle, const char *symbol_name) {
    // Check that handle is valid
    if (!handle) {
        debug_error("Handle invalide");
        return NULL;
    }

    // Cast to your library handle type
    lib_handle_t *lib = (lib_handle_t *) handle;
    // If loader_info exists and has an exported symbols table
    if (lib->exported_symbols) {
        // Look for the symbol in the exported symbols table
        symbol_entry *table = lib->exported_symbols;
        for (int i = 0; table[i].name != NULL; i++) {
            if (strcmp(table[i].name, symbol_name) == 0) {
                // Found the symbol!
                // Check if the address is relative or absolute
                if ((uintptr_t) table[i].addr < (uintptr_t) lib->base_addr) {
                    // Address is relative to the base
                    return (void *) ((char *) lib->base_addr + (uintptr_t) table[i].addr);
                } else {
                    // Address is already absolute
                    return table[i].addr;
                }
            }
        }
    }

    // Symbol not found
    return NULL;
}

/*void* my_dlsym(void* handle, const char* symbol_name) {
    // On vérifie juste que le handle est valide (non NULL)
    if (!handle) {
        debug_error("Handle invalide");
        return NULL;
    }

    // Adresse de base de notre bibliothèque chargée
    extern void* g_base_addr;
    extern elf_header g_hdr;
    extern elf_phdr* g_phdrs; //verifier extern

    // 1. Si e_entry est non nul, essayer d'utiliser la table de symboles
personnalisée if (g_base_addr && g_hdr.e_entry != 0) {
        // Définir le type de notre structure de symbole
        typedef struct {
            const char* name;
            void* addr;
        } symbol_entry;//je le me que soit disponible

        // Définir le type de la fonction get_symbol_table
        typedef symbol_entry* (*get_table_func)();

        // Calculer l'adresse de la fonction get_symbol_table
        get_table_func get_table = (get_table_func)((char*)g_base_addr +
g_hdr.e_entry);

        // Appeler get_symbol_table pour obtenir notre table
        symbol_entry* table = get_table();

        // Parcourir la table de symboles
        for (int i = 0; table[i].name != NULL; i++) {
            if (strcmp(table[i].name, symbol_name) == 0) {
                // On a trouvé le symbole!
                // Mais l'adresse est relative, il faut ajouter l'adresse de
base if ((uintptr_t)table[i].addr < (uintptr_t)g_base_addr) {
                    // Si l'adresse est déjà un offset par rapport à la base
                    return (void*)((char*)g_base_addr +
(uintptr_t)table[i].addr); } else {
                    // Si l'adresse est déjà absolue (cas où la relocation a
déjà été effectuée) return table[i].addr;
                }
      }
        }
    }

    return NULL;
}
*/
int my_set_plt_resolve(void *handle, void *resolve_table) {
    if (!handle) {
        debug_error("Invalid handle");
        return -1;
    }
    lib_handle_t *lib_handle = (lib_handle_t *) handle;

    lib_handle->plt_resolve_table = resolve_table;
    return 0;
}
