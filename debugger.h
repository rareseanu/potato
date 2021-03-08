#include "breakpoint.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

typedef struct debugger_t {
    breakpoint_t breakpoints[5];
    char *child_program_name;
    pid_t child_pid;
} debugger_t;

int initiate_child_trace(const debugger_t *debugger, 
        char** child_process_args);
int run_debugger(const debugger_t *debugger);
void set_breakpoint(unsigned long addr);
breakpoint_t get_breakpoint(const debugger_t *debugger);


