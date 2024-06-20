
#ifndef CDB_RESOLVE
#define CDB_RESOLVE

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

typedef struct FUNC_INFO{
    Dwarf_Signed    linecount;
    Dwarf_Unsigned  lineno;
    Dwarf_Addr      pc;
    Dwarf_Addr      low_pc;
    Dwarf_Addr      high_pc;
    char            *filename;
    char            *func_name;
} FUNC_INFO;

FUNC_INFO *func_info_init();
void func_info_print(FUNC_INFO *f_info);
FUNC_INFO **func_find_all(Dwarf_Debug dbg, int *total);
int func_info_read_from_die(Dwarf_Debug dbg, Dwarf_Die die, 
    FUNC_INFO **func_info);
#endif