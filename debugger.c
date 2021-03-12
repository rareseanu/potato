#include "debugger.h"

void initialize_debugger(size_t initial_breakpoints_size
        , debugger_t *debugger, const char* program_name) {
    debugger->child_program_name = 
        (char*)malloc(sizeof(char) * strlen(program_name));
    strcpy(debugger->child_program_name, program_name) ; 
    debugger->breakpoints = 
        (breakpoint_t*)malloc(sizeof(breakpoint_t) * initial_breakpoints_size);
    debugger->number_of_breakpoints = 0;
    debugger->max_breakpoints = initial_breakpoints_size;
}

int enable_breakpoint(const debugger_t *debugger, breakpoint_t *breakpoint) {

    //Save the word so we can run the original instruction after 
    //disabling the breakpoint.
    breakpoint->word_before_bp = ptrace(PTRACE_PEEKDATA, debugger->child_pid,
            breakpoint->addr, 0);
    printf("\n\tBefore breakpoint enable: %016x", ptrace(PTRACE_PEEKDATA, 
                debugger->child_pid, breakpoint->addr, NULL));  

    //0xcc - INT 3 opcode (software breakpoint)
    //apply the mask on the word so it takes all but the first byte
    //replace the first byte with INT 3 opcode  
    if(ptrace(PTRACE_POKEDATA, debugger->child_pid, breakpoint->addr,
                ((breakpoint->word_before_bp & ~0xff) | 0xcc)) < 0) {
        printf("\n%d", debugger->child_pid);
        printf("\nERROR: ptrace() POKEDATA failure: %s\n", strerror(errno));
        return -1;
    }

    printf("\n\tAfter breakpoint enable: %016x\n", ptrace(PTRACE_PEEKDATA, 
                debugger->child_pid, breakpoint->addr, NULL) );
    return 0;  
}


int add_breakpoint(debugger_t *debugger, unsigned long long addr) {
    if(debugger->number_of_breakpoints == debugger->max_breakpoints) {
        if(debugger->max_breakpoints == 0) {
            debugger->max_breakpoints = 1;
        } else {
            debugger->max_breakpoints *= 2;
        }
        debugger->breakpoints = 
            (breakpoint_t*)realloc(debugger->breakpoints, 
                    sizeof(breakpoint_t) * debugger->max_breakpoints);
    }
    breakpoint_t* breakpoint = (breakpoint_t*)malloc(sizeof(breakpoint_t));
    breakpoint->number = debugger->number_of_breakpoints;
    breakpoint->addr = addr;
    breakpoint->word_before_bp = ptrace(PTRACE_PEEKDATA, debugger->child_pid,
            addr, 0);
    debugger->breakpoints[debugger->number_of_breakpoints++] = *breakpoint;

    enable_breakpoint(debugger, breakpoint);
    return 0;
}



unsigned long long get_register_value(const debugger_t *debugger, 
        char* reg) {
    struct user_regs_struct registers;
    if(ptrace(PTRACE_GETREGS, debugger->child_pid, NULL, &registers)) {
        printf("\nERROR: ptrace() GETREGS failure.");
        return 0;
    }
    if(strcmp(reg, "rip") == 0) {
        return registers.rip;
    } else if(strcmp(reg, "orig_rax") == 0) {
        return registers.orig_rax;
    }

}

int disable_breakpoint(debugger_t *debugger, breakpoint_t *breakpoint) {

    struct user_regs_struct registers;
    if(ptrace(PTRACE_GETREGS, debugger->child_pid, NULL, &registers) <0) {
        printf("\nERROR: ptrace GETREGS failure.");   
        return -1;
    }

    //The instruction pointer now points to the instruction after
    //the trap so we need to change it to point to the 
    //original instruction that we will replace back.
    registers.rip = breakpoint->addr;
    if(ptrace(PTRACE_SETREGS, debugger->child_pid, NULL, &registers) < 0) {
        printf("\nERROR: ptrace SETREGS failure.");
        return -1;
    }

    printf("\n\tBefore breakpoint disable: %016x", ptrace(PTRACE_PEEKDATA, 
                debugger->child_pid, breakpoint->addr, NULL) );

    unsigned long long data_with_int3 = ptrace(PTRACE_PEEKTEXT
            , debugger->child_pid, breakpoint->addr, 0); 
    //Apply the mask on the so it takes all but the first byte that contains
    //INT 3 (0xCC) and replace it with the first byte of the original instruction.
    unsigned long long constructed_back = (data_with_int3 & ~0xff) 
        | (breakpoint->word_before_bp & 0xff);

    if(ptrace(PTRACE_POKETEXT, debugger->child_pid, breakpoint->addr,
                constructed_back) < 0) {
        printf("\nERROR: ptrace POKEDATA\n");
        return -1;
    }
    printf("\n\tAfter breakpoint disable: %016x", ptrace(PTRACE_PEEKDATA, 
                debugger->child_pid, breakpoint->addr, NULL));
    return 0;
}

int initiate_child_trace(const debugger_t *debugger, 
        char** child_process_args) {
    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
        printf("\nERROR: Unsuccessful ptrace() call.\n");
        return -1;
    }
    execve(debugger->child_program_name, child_process_args, NULL);
}

void dump(long data) {
    for(int i = 0; i < 64; i+= 8) {
        printf("%02x ", ((long long) data >> i) & 0xff);
    }
}

breakpoint_t* get_breakpoint(const debugger_t *debugger,
        unsigned long long address) {
    for(int i = 0; i < debugger->number_of_breakpoints; ++i) {
        unsigned long long temp = debugger->breakpoints[i].addr;
        if(temp == address) {
            return &(debugger->breakpoints[i]);
        }
    }
    return NULL;
}

int run_debugger(debugger_t *debugger) {
    printf("Debug on pid: %d\n", debugger->child_pid);
    int status;
    char command[50];
    struct user_regs_struct registers;

    bool disable_last_bp = false;
    breakpoint_t * bp = NULL;
    unsigned long long rip = 0;
    // waitpid() - wait for state changes in the given child process
    waitpid(debugger->child_pid, &status, 0);
    while(1) {
        if(WIFEXITED(status)) {
            printf("\nChild process exited with code %d\n", WEXITSTATUS(status));
            return 0;
        } else if(WIFSIGNALED(status)) {
            printf("\nChild process killed by signal %d\n", WTERMSIG(status));
            return -1;
        }

        printf("\ndebugger > ");
        scanf("%s", command);

        // ptrace() - observe/control the execution of another process 
        if(strcmp(command, "info_rip") == 0) {
            ptrace(PTRACE_GETREGS, debugger->child_pid, NULL, &registers);
            ptrace(PTRACE_PEEKUSER, debugger->child_pid
                    ,registers.rip, 0);
            printf("\n[RIP] %lx : ", registers.rip);
            continue;
            //dump((unsigned long) ptrace(PTRACE_PEEKTEXT, pid, registers.rip, 0)
        } else if(strcmp(command, "breakpoint") == 0) {
            unsigned long long address;
            printf("\nEnter the address without \"0x\":  ");
            scanf("%llx", &address);

            add_breakpoint(debugger, address);
            continue;
        } else if(strcmp(command, "disable_bp") == 0) {
            unsigned long long address;
            printf("\nEnter the address without \"0x\": ");
            scanf("%x", &address);
            //disable_breakpoint(debugger, address);
            continue;
        }

        if(WSTOPSIG(status) == SIGTRAP && WIFSTOPPED(status)) {
            if(strcmp(command, "next") == 0) {
                if(disable_last_bp) {
                    disable_last_bp = false;
                    enable_breakpoint(debugger, bp);
                    if(ptrace(PTRACE_SINGLESTEP, debugger->child_pid
                                , NULL, NULL) < 0) {
                        printf("\nERROR: ptrace() SINGLESTEP failure.");
                        continue;
                    }
                    waitpid(debugger->child_pid, &status, 0);
                } 

                if(ptrace(PTRACE_SINGLESTEP, debugger->child_pid
                            , NULL, NULL) < 0) {
                    printf("\nERROR: ptrace() SINGLESTEP failure.");
                    continue;
                }
                waitpid(debugger->child_pid, &status, 0);
            } else if(strcmp(command, "continue") == 0) {
                if(disable_last_bp) {
                    //In case we had had a breakpoint right before, 
                    //single step the original instruction we
                    //just replaced back and enable the breakpoint again.
                    if(ptrace(PTRACE_SINGLESTEP, debugger->child_pid, NULL, NULL)) {
                        printf("\nERROR: ptrace() SINGLESTEp failure.\n");
                    }
                    waitpid(debugger->child_pid, &status, 0);
                    if(WIFEXITED(status)) {
                        printf("\nChild process exited with code %d\n"
                                , WEXITSTATUS(status));
                        return 0;
                    }

                    disable_last_bp = false;
                    enable_breakpoint(debugger, bp);
                }

                if(ptrace(PTRACE_CONT, debugger->child_pid, NULL, NULL) < 0) {
                    printf("\nERROR: ptrace() CONTINUE failure.");
                }

                waitpid(debugger->child_pid, &status, 0);
                if(WIFEXITED(status)) {
                    printf("\nChild process exited with code %d\n"
                            , WEXITSTATUS(status));
                    return 0;
                }
            } else {
                printf("\nCommand not found.\n");
                continue;
            }

            //Check for breakpoints at (instruction pointer) - 1 as
            //the INT 3 instruction was already executed.
            rip = get_register_value(debugger, "rip");
            bp = get_breakpoint(debugger, rip - 1); 
            if(bp != NULL) {
                printf("\nBreakpoint %d at RIP: %llx\n",bp->number, rip-1);
                disable_breakpoint(debugger, bp);
                disable_last_bp = true;
            } 

        }
    }
}


