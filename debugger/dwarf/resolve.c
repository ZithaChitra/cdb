#include <stdio.h>
#include <stdlib.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <string.h>
#include "resolve.h"

void func_info_print(FUNCINFO *f_info)
{
    printf("name: %s, lowpc: %llx\n", f_info->func_name, f_info->low_pc);
    printf("------------------------\n\n");
}

FUNCINFO *func_info_init()
{
    FUNCINFO *func_info = (FUNCINFO *)malloc(sizeof(FUNCINFO));
    if(func_info == NULL) return NULL;
    func_info->filename  = NULL;
    func_info->func_name = NULL;
    func_info->linecount = 0;
    func_info->low_pc    = 0;
    func_info->high_pc   = 0;
    return func_info;
}
int func_info_read_from_die(Dwarf_Debug dbg, Dwarf_Die die, 
    FUNCINFO **func_info)
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

FUNCINFO **func_find_all(Dwarf_Debug dbg, int *total)
{
    Dwarf_Error err;
    Dwarf_Die no_die = 0, cu_die, child_die;
    Dwarf_Unsigned curr_header_len, abbrev_offset, next_cu_header;
    Dwarf_Half version_stamp, address_size;
    size_t curs = 0;

    // lets assume a max of 10 funcs for now
    int max_funcs = 10;
    FUNCINFO **all_funcs = (FUNCINFO **)malloc(sizeof(FUNCINFO *) * max_funcs);
    if(all_funcs == NULL) return NULL;
    

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
                FUNCINFO *func_info = NULL;
                if(func_info_read_from_die(dbg, child_die, &func_info) == -1) continue;
                if(func_info == NULL || func_info->low_pc == 0) continue;
                if(curs >= max_funcs) break;
                all_funcs[curs] = func_info;
                curs++;
            }while(dwarf_siblingof(dbg, child_die, &child_die, &err) == DW_DLV_OK);
        }
        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    }
    *total = curs;
    return all_funcs;
}


LINEINFO get_file_line_from_address(Dwarf_Debug dbg, Dwarf_Addr absolute_address, Dwarf_Addr base_address) {
    LINEINFO result = {NULL, 0};
    Dwarf_Error error = NULL;
    Dwarf_Arange *aranges;
    Dwarf_Signed arange_count;
    Dwarf_Off cu_die_offset;
    Dwarf_Die cu_die = NULL;
    Dwarf_Line *lines = NULL;
    Dwarf_Signed line_count = 0;

        // Convert absolute address to relative
    Dwarf_Addr relative_address = absolute_address - base_address;

    // Get aranges
    if (dwarf_get_aranges(dbg, &aranges, &arange_count, &error) != DW_DLV_OK) {
        return result;
    }

    // Find the arange containing our address
    Dwarf_Arange arange;
    if (dwarf_get_arange(aranges, arange_count, relative_address, &arange, &error) != DW_DLV_OK) {
        dwarf_dealloc(dbg, aranges, DW_DLA_LIST);
        return result;
    }

    // Get the CU DIE offset from the arange
    if (dwarf_get_cu_die_offset(arange, &cu_die_offset, &error) != DW_DLV_OK) {
        fprintf(stderr, "%s\n", dwarf_errmsg(error));
        dwarf_dealloc(dbg, aranges, DW_DLA_LIST);
        return result;
    }

    // Get the CU DIE
    if (dwarf_offdie(dbg, cu_die_offset, &cu_die, &error) != DW_DLV_OK) {
        fprintf(stderr, "%s\n", dwarf_errmsg(error));
        dwarf_dealloc(dbg, aranges, DW_DLA_LIST);
        return result;
    }

    // Get all the lines for this CU
    if (dwarf_srclines(cu_die, &lines, &line_count, &error) == DW_DLV_OK) {
        for (int i = 0; i < line_count; i++) {
            Dwarf_Addr line_addr;
            if (dwarf_lineaddr(lines[i], &line_addr, &error) == DW_DLV_OK) {
                if (line_addr == relative_address) {
                    // We found the matching line, get filename and line number
                    char *filename;
                    if (dwarf_linesrc(lines[i], &filename, &error) == DW_DLV_OK) {
                        result.filename = strdup(filename);
                        dwarf_dealloc(dbg, filename, DW_DLA_STRING);
                    }
                    Dwarf_Unsigned line_number;
                    if (dwarf_lineno(lines[i], &line_number, &error) == DW_DLV_OK) {
                        result.line_number = (unsigned int)line_number;
                    }
                    break;
                }
            }
        }
        dwarf_dealloc(dbg, lines, DW_DLA_LIST);
    }

    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
    dwarf_dealloc(dbg, aranges, DW_DLA_LIST);
    return result;
}
