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

void add_breakpoint(debugger_t *debugger, unsigned long long addr) {
    if(debugger->number_of_breakpoints == debugger->max_breakpoints) {
        if(debugger->max_breakpoints == 0) {
            debugger->max_breakpoints = 1;
        } else {
            debugger->max_breakpoints *= 2;
        }
        debugger->breakpoints = 
            (breakpoint_t*)realloc(debugger->breakpoints, 
                    sizeof(breakpoint_t) * debugger->max_breakpoints);
        printf("\nNew size: %d", debugger->max_breakpoints);
    }
    breakpoint_t* breakpoint = (breakpoint_t*)malloc(sizeof(breakpoint_t));
    breakpoint->number = debugger->number_of_breakpoints;
    breakpoint->addr = addr;
    printf("\nBefore breakpoint: ");
    unsigned long long word = ptrace(PTRACE_PEEKDATA, 
            debugger->child_pid, breakpoint->addr, NULL);
    //Save the word so it can be replaced back after receiving the SIGTRAP
    breakpoint->word_before_bp = word;

    printf("\nData at 0x%08x : 0x%016x\n", breakpoint->addr, 
            breakpoint->word_before_bp);
    //0xcc - INT 3 opcode (software breakpoint)
    //apply the mask on the word so it takes all but the first byte
    //replace the first byte with INT 3 opcode  
    ptrace(PTRACE_POKEDATA, debugger->child_pid, breakpoint->addr, 
            ((word & ~0xff) | 0xcc));

    unsigned long long word_after_bp = ptrace(PTRACE_PEEKDATA, 
            debugger->child_pid, breakpoint->addr, NULL);

    printf("\nAfter breakpoint: ");

    printf("\nData at 0x%08x : 0x%016x\n", breakpoint->addr, word_after_bp);
    debugger->breakpoints[debugger->number_of_breakpoints++] = *breakpoint;
}

void enable_breakpoint(const debugger_t *debugger,
        const breakpoint_t *breakpoint) {
    if(ptrace(PTRACE_POKEDATA, debugger->child_pid, breakpoint->addr,
            ((breakpoint->word_before_bp & ~0xff) | 0xcc)) < 0) {
        printf("\n%d", debugger->child_pid);
        printf("\nERROR: ptrace() POKEDATA failure: %s\n", strerror(errno));
        return;
    }

}

void disable_breakpoint(debugger_t *debugger, unsigned long long addr) {
    for(int i = 0; i < debugger->number_of_breakpoints; ++i) {
        unsigned long long temp = debugger->breakpoints[i].addr;
        if(temp == addr) {
            struct user_regs_struct registers;
            ptrace(PTRACE_GETREGS, debugger->child_pid, NULL, &registers);
            ptrace(PTRACE_POKEDATA, debugger->child_pid, temp,
                    debugger->breakpoints[i].word_before_bp);
            //The instruction pointer now points to the instruction after
            //the trap so we need to change it to point to the 
            //original instruction we just replaced back.
            registers.rip -= 1;
            ptrace(PTRACE_SETREGS, debugger->child_pid, 0, &registers);
            unsigned long long word_after_bp = ptrace(PTRACE_PEEKDATA, 
                    debugger->child_pid, debugger->breakpoints[i].addr, NULL);
            printf("\nData at 0x%08x : 0x%016x\n",
                    debugger->breakpoints[i].addr, word_after_bp);

            printf("\nDisabled breakpoint at addr: %x", addr);
            break;
        }
    } 
}


unsigned long long get_register_value(const debugger_t *debugger, 
        char* reg) {
    struct user_regs_struct registers;
    ptrace(PTRACE_GETREGS, debugger->child_pid, NULL, &registers);
    if(strcmp(reg, "rip") == 0) {
        return registers.rip;
    } else if(strcmp(reg, "orig_rax") == 0) {
        return registers.orig_rax;
    }

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
    unsigned long long instruction_pointer;
    bool reenable_last_bp = false;
    breakpoint_t  *last_breakpoint = NULL;
    siginfo_t signal_info;
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
            disable_breakpoint(debugger, address);
            continue;
        }
        

        // TODO: FIX bp not being properly re-enabled after executing the
        // operation that it replaces on PTRACE_CONT.
        if(WSTOPSIG(status) == SIGTRAP && WIFSTOPPED(status)) {           
            

            if(strcmp(command, "next") == 0) {
                if(ptrace(PTRACE_SINGLESTEP, debugger->child_pid
                            , NULL, NULL) < 0) {
                    printf("\nERROR: ptrace() SINGLESTEP failure.");   
                }
                
            } else if(strcmp(command, "continue") == 0) {
                if(reenable_last_bp == true) {
                    ptrace(PTRACE_SINGLESTEP, debugger->child_pid,
                            NULL, NULL);
                    waitpid(debugger->child_pid, &status,0);
                    if(WIFEXITED(status)) {
                        return 0;
                    } 
                    enable_breakpoint(debugger, last_breakpoint);
                    reenable_last_bp = false;
                    unsigned long long word_after_bp = ptrace(PTRACE_PEEKDATA, 
                            debugger->child_pid, last_breakpoint->addr, NULL);
                    printf("\nAfter reenable: %016x\n", word_after_bp);
                    
                }
                if(ptrace(PTRACE_CONT, debugger->child_pid, NULL, NULL) < 0) {
                    printf("\nERROR: ptrace() CONTINUE failure.");
                }
            } else {
                printf("\nCommand not found.\n");
                continue;
            }
            waitpid(debugger->child_pid, &status, 0);
            if(reenable_last_bp == true && !WIFEXITED(status)) {
                enable_breakpoint(debugger, last_breakpoint);
                reenable_last_bp = false;
                unsigned long long word_after_bp = ptrace(PTRACE_PEEKDATA, 
                        debugger->child_pid, last_breakpoint->addr, NULL);
                printf("\nAfter reenable: %016x\n", word_after_bp); 
            }
            instruction_pointer = get_register_value(debugger, "rip");
            last_breakpoint = 
                get_breakpoint(debugger, instruction_pointer - 1);
            if(last_breakpoint != NULL) {
                printf("\nBreakpoint %d at current address %x\n"
                        , last_breakpoint->number, last_breakpoint->addr);
                disable_breakpoint(debugger, last_breakpoint->addr);
                reenable_last_bp = true;
            }

        } 
    }
}

