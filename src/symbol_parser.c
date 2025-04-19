#include "../include/elf_parser.h"
#include "../include/debug.h"
#include <string.h>
#include <stdlib.h>

// Types de symboles ELF
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

#define STT_FUNC   2

// Macros pour extraire les infos de st_info
#define ELF64_ST_BIND(i)   ((i)>>4)
#define ELF64_ST_TYPE(i)   ((i)&0xf)

// Au lieu de chercher via la table d'exports, cherchons via la table de symboles dynamiques
int find_dynamic_symbol(void* base_addr, elf_header* hdr, elf_phdr* phdrs, 
                    const char* name, void** symbol_addr) {
    debug_detail("Recherche du symbole dans la table dynamique");
    
    // Trouver le segment dynamique
    elf_phdr* dyn_seg = NULL;
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_seg = &phdrs[i];
            debug_detail("Segment PT_DYNAMIC trouvé");
            break;
        }
    }
    
    if (!dyn_seg) {
        debug_error("Pas de segment PT_DYNAMIC trouvé");
        return -1;
    }
    
    // Tables nécessaires pour résoudre les symboles
    Elf64_Sym* symtab = NULL;
    char* strtab = NULL;
    
    // Parcourir la section dynamique
    uint64_t* dynamic = (uint64_t*)((char*)base_addr + dyn_seg->p_vaddr);
    int i = 0;
    
    while (dynamic[i] != 0) {
        uint64_t tag = dynamic[i++];
        uint64_t val = dynamic[i++];
        
        debug_verbose("Analyse entrée dynamique");
        
        if (tag == 6) { // DT_SYMTAB
            symtab = (Elf64_Sym*)((char*)base_addr + val);
            debug_detail("Table de symboles trouvée");
        }
        else if (tag == 5) { // DT_STRTAB
            strtab = (char*)base_addr + val;
            debug_detail("Table de chaînes trouvée");
        }
    }
    
    if (!symtab || !strtab) {
        debug_error("Tables de symboles ou de chaînes non trouvées");
        return -1;
    }
    
    // Parcourir les symboles (limité à un nombre raisonnable)
    for (i = 0; i < 1000; i++) {
        Elf64_Sym sym = symtab[i];
        
        if (sym.st_name == 0) {
            // Symbole sans nom, continuer
            continue;
        }
        
        // Vérification de sécurité avant d'accéder à strtab
        if (sym.st_name >= 1000000) {  // Limite arbitraire raisonnable
            debug_warn("Index de chaîne suspect, ignoré");
            continue;
        }
        
        char* sym_name = strtab + sym.st_name;
        
        // Vérification basique que le nom est valide
        int valid = 1;
        for (int j = 0; j < 100 && sym_name[j] != '\0'; j++) {
            if (sym_name[j] < 32 || sym_name[j] > 126) { // Caractères non imprimables
                valid = 0;
                break;
            }
        }
        
        if (!valid) {
            continue;
        }
        
       if (strncmp(sym_name, name, strlen(name)) == 0 && 
            (sym_name[strlen(name)] == '\0' || sym_name[strlen(name)] == '@')) {
            // Trouvé! (either exact match or versioned symbol)
            debug_info("Fonction trouvée dans la table de symboles");
            *symbol_addr = (void*)((char*)base_addr + sym.st_value);
            return 0;
        }
        
        // Si on atteint un symbole sans nom après un certain nombre,
        // on peut supposer qu'on a atteint la fin
        if (i > 100 && sym.st_name == 0) {
            break;
        }
    }
    debug_warn("Symbole non trouvé");
    return -1;
}