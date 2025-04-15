#include "../include/elf_parser.h"
#include "../include/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int perform_relocations(void* base_addr, elf_header* hdr, elf_phdr* phdrs) {
    debug_info("Début des relocations");

    elf_phdr* dyn_segment = NULL;
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_segment = &phdrs[i];
            debug_detail("Segment PT_DYNAMIC trouvé");
            break;
        }
    }
    
    if (!dyn_segment) {
        debug_info("Aucun segment dynamique trouvé");
        return 0;
    }
    
    Elf64_Rela* rela = NULL;
    int rela_count = 0;
    
    uint64_t* dynamic = (uint64_t*)((char*)base_addr + dyn_segment->p_vaddr);
    debug_detail("Section dynamique trouvée");
    
    int i = 0;
    
    while (dynamic[i] != 0) {
        uint64_t tag = dynamic[i++];
        uint64_t val = dynamic[i++];
        
        debug_verbose("Entrée dynamique trouvée");
        
        if (tag == 7) {  // DT_RELA
            rela = (Elf64_Rela*)((char*)base_addr + val);
            debug_detail("Table RELA trouvée");
        }
        else if (tag == 8) {  // DT_RELASZ
            rela_count = val / sizeof(Elf64_Rela);
            debug_detail("Taille table RELA trouvée");
        }
    }
    
    if (!rela || rela_count == 0) {
        debug_info("Aucune relocation trouvée");
        return 0;
    }
    
    debug_info("Traitement des relocations");
    
    for (int i = 0; i < rela_count; i++) {
        uint64_t offset = rela[i].r_offset;
        uint32_t type = rela[i].r_info & 0xffffffff;
        
        debug_verbose("Relocation trouvée");
        
        if (type == R_X86_64_RELATIVE) {
            uint64_t* target = (uint64_t*)((char*)base_addr + offset);
            debug_verbose("Relocation de type RELATIVE");
            
            uint64_t new_value = (uint64_t)base_addr + rela[i].r_addend;
            *target = new_value;
            
            debug_verbose("Relocation appliquée");
        }
    }
    
    debug_info("Relocations terminées");
    return 0;
}