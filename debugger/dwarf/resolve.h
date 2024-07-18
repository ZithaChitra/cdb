
#ifndef CDB_RESOLVE
#define CDB_RESOLVE

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

typedef struct FUNCINFO{
    Dwarf_Signed    linecount;
    Dwarf_Unsigned  lineno;
    Dwarf_Addr      pc;
    Dwarf_Addr      low_pc;
    Dwarf_Addr      high_pc;
    char            *filename;
    char            *func_name;
} FUNCINFO;


typedef struct LINEINFO
{
    char         *filename;
    unsigned int line_number;
} LINEINFO;


FUNCINFO *func_info_init();
LINEINFO get_file_line_from_address(
        Dwarf_Debug dbg, 
        Dwarf_Addr absolute_address, 
        Dwarf_Addr base_address);
void func_info_print(FUNCINFO *f_info);
FUNCINFO **func_find_all(Dwarf_Debug dbg, int *total);
int func_info_read_from_die(Dwarf_Debug dbg, Dwarf_Die die, 
    FUNCINFO **func_info);

#endif