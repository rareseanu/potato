#include "breakpoint.h"
#include "dwarf_utils.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/personality.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <elf.h>

typedef struct debugger_t {
    breakpoint_t *breakpoints;
    char *child_program_name;
    pid_t child_pid;
    size_t number_of_breakpoints;
    size_t max_breakpoints;
    Elf64_Ehdr elf_header;
    unsigned long long load_address;
} debugger_t;

void read_elf_header(const char* elfFile, debugger_t *debugger);
void read_load_address(debugger_t *debugger);

void initialize_debugger(size_t initial_size, debugger_t *debugger,
        const char* program_name);
int initiate_child_trace(const debugger_t *debugger, 
        char** child_process_args);
void dump(long data);

int run_debugger(debugger_t *debugger);
int add_breakpoint(debugger_t *debugger, unsigned long long addr);


