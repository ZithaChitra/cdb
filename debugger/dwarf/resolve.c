#include <stdio.h>
#include <stdlib.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include "resolve.h"

void func_info_print(FUNC_INFO *f_info)
{
    printf("name: %s, lowpc: %llx\n", f_info->func_name, f_info->low_pc);
    printf("------------------------\n\n");
}

FUNC_INFO *func_info_init()
{
    FUNC_INFO *func_info = (FUNC_INFO *)malloc(sizeof(FUNC_INFO));
    if(func_info == NULL) return NULL;
    func_info->filename  = NULL;
    func_info->func_name = NULL;
    func_info->linecount = 0;
    func_info->low_pc    = 0;
    func_info->high_pc   = 0;
    return func_info;
}
int func_info_read_from_die(Dwarf_Debug dbg, Dwarf_Die die, 
    FUNC_INFO **func_info)
{
    Dwarf_Half  tag;
    Dwarf_Error err = NULL;
    if((dwarf_tag(die, &tag, &err) == DW_DLV_OK) && tag == DW_TAG_subprogram)
    {
        Dwarf_Line *linebuf;
        *func_info = func_info_init();
        if(func_info == NULL) return -1;

        if((dwarf_diename(die, &(*func_info)->func_name, &err) == DW_DLV_OK) 
            && (*func_info)->func_name){
        }
        printf("func_name: %s\n", (*func_info)->func_name);
        Dwarf_Addr low_pc;
        if(dwarf_lowpc(die, &low_pc, &err) == DW_DLV_OK)
        {
            printf("setting low pc: %lld\n", low_pc);
            (*func_info)->low_pc = low_pc;
        }else{
            printf("error: %s\n", dwarf_errmsg(err));
            return -1;
        }

        // if(dwarf_srclines(die, &linebuf, 
        //     &(*func_info)->linecount, &err) != DW_DLV_OK)
        // {
        //     printf("errno: %lld\n", dwarf_errno(err));
        //     fprintf(stderr, "Failed to get source lines: %s\n", dwarf_errmsg(err));
        //     free((*func_info));
        //     return -1;
        // }
        // for (Dwarf_Signed i = 0; i < (*func_info)->linecount; i++)
        // {
        //     if((dwarf_lineno(linebuf[i], &(*func_info)->lineno, &err) == DW_DLV_OK) &&
        //         (dwarf_lineaddr(linebuf[i], &(*func_info)->pc, &err) == DW_DLV_OK) && 
        //         (dwarf_linesrc(linebuf[i], &(*func_info)->filename, &err) == DW_DLV_OK))
        //     {
        //         printf("File: %s Line: %llu Address: 0x%llx\n", 
        //             (*func_info)->filename, (*func_info)->lineno, (*func_info)->pc);
        //     }
        // }
        // dwarf_srclines_dealloc(dbg, linebuf, (*func_info)->linecount);
        return 0;
    }
    return -1;
}

int func_find_all(Dwarf_Debug dbg, FUNC_INFO **all_funcs, int *total)
{
    Dwarf_Error err;
    Dwarf_Die no_die = 0, cu_die, child_die;
    Dwarf_Unsigned curr_header_len, abbrev_offset, next_cu_header;
    Dwarf_Half version_stamp, address_size;
    size_t curs = 0;

    // lets assume a max of 10 funcs for now
    int max_funcs = 10;
    *all_funcs = (FUNC_INFO *)malloc(sizeof(FUNC_INFO *) * max_funcs);
    if(*all_funcs == NULL) return -1;
    

    while(dwarf_next_cu_header(dbg, &curr_header_len, &version_stamp,
            &abbrev_offset, &address_size, &next_cu_header, &err) == DW_DLV_OK)
    {
        if(dwarf_siblingof(dbg, no_die, &cu_die, &err) != DW_DLV_OK)
        {
            fprintf(stderr, "dwarf_siblingof failed: %s\n", dwarf_errmsg(err));
            continue;
        }
        if(dwarf_child(cu_die, &child_die, &err) == DW_DLV_OK)
        {
            do{
                FUNC_INFO *func_info = NULL;
                if(func_info_read_from_die(dbg, child_die, &func_info) == -1) continue;
                if(func_info == NULL || func_info->low_pc == 0) continue;
                if(curs >= max_funcs) break;
                *(*all_funcs + curs) = *func_info;
                curs++;
            }while(dwarf_siblingof(dbg, child_die, &child_die, &err) == DW_DLV_OK);
        }
        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }
    *total = curs;
    return 0;
}

