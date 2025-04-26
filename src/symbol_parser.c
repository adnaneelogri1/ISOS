#include "elf_parser.h"
#include "debug.h"
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

// Tags pour la section dynamic
#define DT_NULL     0
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8

int find_dynamic_symbol(void* base_addr, elf_header* hdr, elf_phdr* phdrs, 
                    const char* name, void** symbol_addr) {
    debug_detail("Recherche du symbole dans la table dynamique");
    
    if (!base_addr || !hdr || !phdrs || !name || !symbol_addr) {
        debug_error("Arguments invalides pour find_dynamic_symbol");
        return -1;
    }
    
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
    if (!dynamic) {
        debug_error("Pointeur de section dynamique invalide");
        return -1;
    }
    
    //debug_detail("Section dynamique trouvée à l'adresse relative 0x%lx", dyn_seg->p_vaddr);
    
    int i = 0;
    // Limiter la recherche pour éviter une boucle infinie
    int max_entries = 100;
    
    while (i < max_entries * 2 && dynamic[i] != DT_NULL) {
        uint64_t tag = dynamic[i++];
        uint64_t val = dynamic[i++];
        
        //debug_verbose("Entrée dynamique trouvée: tag=%lu, val=0x%lx", tag, val);
        
        if (tag == DT_SYMTAB) {
            symtab = (Elf64_Sym*)((char*)base_addr + val); 
            //debug_detail("Table de symboles trouvée à l'offset 0x%lx", val);
        }
        else if (tag == DT_STRTAB) {
            strtab = (char*)base_addr + val;
           // debug_detail("Table de chaînes trouvée à l'offset 0x%lx", val);
        }
    }
    
    if (!symtab || !strtab) {
        debug_error("Tables de symboles ou de chaînes non trouvées");
        return -1;
    }
    
    //debug_detail("Début de la recherche du symbole: %s", name);
    
    // Parcourir les symboles (limité à un nombre raisonnable)
    int max_symbols = 100;  // Limite raisonnable
    for (i = 0; i < max_symbols; i++) {
        // Vérifions si l'adresse est valide avant d'y accéder
        Elf64_Sym* sym_ptr = &symtab[i];
        
        if (!sym_ptr) {
            debug_error("Adresse de symbole invalide");
            break;
        }
        
        Elf64_Sym sym = *sym_ptr;
        
        if (sym.st_name == 0) {
            // Symbole sans nom, continuer
            continue;
        }
        
        // Vérification de sécurité avant d'accéder à strtab
        if (sym.st_name >= 10000) {  // Limite plus raisonnable
            //debug_warn("Index de chaîne suspect (%u), ignoré", sym.st_name);
            continue;
        }
        
        // Vérifier que strtab+st_name est une adresse valide
        char* sym_name = strtab + sym.st_name;
        if (!sym_name) {
            debug_error("Adresse de nom de symbole invalide");
            continue;
        }
        
        // Essayons d'abord de voir si le premier caractère est valide
        if (sym_name[0] < 32 || sym_name[0] > 126) {
            //debug_verbose("Nom de symbole invalide, premier caractère: %d", sym_name[0]);
            continue;
        }
        
        // Nous avons au moins un caractère valide, vérifions le reste plus prudemment
        size_t name_len = strlen(name);
        int match = 1;
        
        // Vérifier caractère par caractère sans dépasser la taille de name
        for (size_t j = 0; j < name_len; j++) {
            if (sym_name[j] == '\0' || sym_name[j] != name[j]) {
                match = 0;
                break;
            }
        }
        
        // Vérifier que le symbole se termine correctement
        if (match && (sym_name[name_len] == '\0' || sym_name[name_len] == '@')) {
            //debug_info("Fonction '%s' trouvée dans la table de symboles, valeur=0x%lx", name, sym.st_value);
                       
            // Vérifier que l'adresse calculée est valide
            if (sym.st_value == 0) {
                debug_error("Symbole trouvé mais avec une valeur nulle");
                return -1;
            }
            
            *symbol_addr = (void*)((char*)base_addr + sym.st_value);
            return 0;
        }
    }
    
    //debug_warn("Symbole '%s' non trouvé après %d entrées", name, max_symbols);
    return -1;
}