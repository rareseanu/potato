#include <dwarf.h>
#include <libdwarf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // exit()
#include <string.h> // strcmp()
#include <unistd.h> // close()
#include <fcntl.h> // open()

unsigned long long process_cu(char* program_name, char* function_name);
unsigned long long get_address_from_function(Dwarf_Debug dbg, Dwarf_Die die);
bool is_function(Dwarf_Debug dgb, Dwarf_Die die, char *function_name);