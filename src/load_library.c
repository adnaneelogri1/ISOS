#include "elf_parser.h"
#include "debug.h"
#include <stdio.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h> /* memset */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>



static void* memcpy_safe(void *dest, const void *src, size_t n) {
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    
    // Vérifier le chevauchement (overlap) des zones mémoire
    if ((d <= s && d + n > s) || (s <= d && s + n > d)) {
        // Chevauchement détecté, on copie de droite à gauche
        for (size_t i = n; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    } else {
        // Pas de chevauchement, on copie de gauche à droite
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }
    
    return dest;
}

// Variables globales pour stocker l'état de la bibliothèque chargée
void *g_base_addr = NULL;
elf_header g_hdr;
elf_phdr *g_phdrs = NULL;

int load_library(int fd, elf_header *hdr, elf_phdr *phdrs, void **out_base_addr) {
    size_t page_size = getpagesize();

    // Trouver l'étendue des segments de chargement
    uint64_t min_addr = UINT64_MAX;
    uint64_t max_addr = 0;
    int load_segments = 0;

    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            load_segments++;

            if (phdrs[i].p_vaddr < min_addr)
                min_addr = phdrs[i].p_vaddr;

            uint64_t seg_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            if (seg_end > max_addr)
                max_addr = seg_end;
        }
    }

    if (load_segments == 0) {
        debug_error("Pas de segments PT_LOAD trouvés");
        return -1;
    }

    // Aligner sur la taille de page
    uint64_t base_offset = min_addr & ~(page_size - 1);

    size_t total_size = max_addr - base_offset;
    total_size = (total_size + page_size - 1) & ~(page_size - 1);

    // Réserver la mémoire (non accessible initialement)
    void *base_addr = mmap(
        NULL,
        total_size,
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (base_addr == MAP_FAILED) {
        debug_error("mmap initial a échoué");
        return -1;
    }

    // Calculer l'adresse de base virtuelle
    uint64_t base_address = (uint64_t) base_addr - base_offset;
    debug_info("Adresse de base pour chargement");

    // Charger tous les segments PT_LOAD
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint64_t seg_vaddr = phdrs[i].p_vaddr;
            uint64_t aligned_vaddr = seg_vaddr & ~(page_size - 1);
            uint64_t offset_in_page = seg_vaddr - aligned_vaddr;

            void *load_addr = (void *) (base_address + aligned_vaddr);

            if ((uint64_t) load_addr % page_size != 0) {
                debug_error("Adresse pas alignée sur une page");
                munmap(base_addr, total_size);
                return -1;
            }

            size_t file_size = phdrs[i].p_filesz;
            size_t mem_size = phdrs[i].p_memsz;

            size_t raw_size = mem_size + offset_in_page;
            size_t aligned_size = (raw_size + page_size - 1) & ~(page_size - 1);

            debug_detail("Chargement segment PT_LOAD");

            // Mapper la partie du fichier en mémoire
            if (file_size > 0) {
                void *segment_addr = mmap(
                    load_addr,
                    file_size + offset_in_page,
                    PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_PRIVATE,
                    fd,
                    phdrs[i].p_offset & ~(page_size - 1)
                );

                if (segment_addr == MAP_FAILED) {
                    debug_error("mmap segment a échoué");
                    munmap(base_addr, total_size);
                    return -1;
                }
            }

            // Initialiser la section BSS (mémoire sans fichier)
            if (mem_size > file_size) {
                void *bss_start = (void *) ((char *) load_addr + offset_in_page + file_size);
                size_t bss_size = mem_size - file_size;

                debug_detail("Initialisation BSS");

                // S'assurer qu'on peut écrire dans cette zone mémoire
                if (mprotect(load_addr, aligned_size, PROT_READ | PROT_WRITE) != 0) {
                    debug_error("mprotect pour BSS a échoué");
                    munmap(base_addr, total_size);
                    return -1;
                }

                // Mettre à zéro la section BSS
                explicit_bzero(bss_start, bss_size);
            }
        }
    }

    // Avant les relocations, tous les segments sont déjà PROT_READ | PROT_WRITE
    debug_info("Exécution des relocations...");
    if (perform_relocations((void *) base_address, hdr, phdrs) != 0) {
        debug_error("Échec des relocations");
        munmap(base_addr, total_size);
        return -1;
    }

    // Après les relocations, appliquer les protections finales
    debug_info("Application des protections finales...");
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint64_t seg_vaddr = phdrs[i].p_vaddr;
            uint64_t aligned_vaddr = seg_vaddr & ~(page_size - 1);
            uint64_t offset_in_page = seg_vaddr - aligned_vaddr;
            void *load_addr = (void *) (base_address + aligned_vaddr);

            size_t raw_size = phdrs[i].p_memsz + offset_in_page;
            size_t aligned_size = (raw_size + page_size - 1) & ~(page_size - 1);

            // Déterminer les permissions finales
            int prot = 0;
            if (phdrs[i].p_flags & PF_R) prot |= PROT_READ;
            if (phdrs[i].p_flags & PF_W) prot |= PROT_WRITE;
            if (phdrs[i].p_flags & PF_X) prot |= PROT_EXEC;

            debug_detail("Protection finale segment");

            if (mprotect(load_addr, aligned_size, prot) != 0) {
                debug_error("mprotect final a échoué");
                munmap(base_addr, total_size);
                return -1;
            }
        }
    }

    // Enregistrer l'état global de la bibliothèque
    g_base_addr = (void *) base_address;
    memcpy_safe(&g_hdr, hdr, sizeof(elf_header));
    // On libère l'ancien si nécessaire
    if (g_phdrs) {
        free(g_phdrs);
    }

    // On copie les headers de programme
  if (hdr->e_phnum > 0) {
    g_phdrs = malloc(hdr->e_phnum * sizeof(elf_phdr));
    if (g_phdrs) {
        memcpy_safe(g_phdrs, phdrs, hdr->e_phnum * sizeof(elf_phdr));
    }
    } else {
        g_phdrs = NULL;  // Pas de headers de programme à copier
    }
    *out_base_addr = (void *) base_address;
    return 0;
}
