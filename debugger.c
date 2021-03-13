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

void clean_debugger(debugger_t *debugger) {
    free(debugger->child_program_name);
    free(debugger->breakpoints);
}

void read_elf_header(const char* elfFile, debugger_t *debugger) {
    FILE* file = fopen(elfFile, "rb");
    Elf64_Ehdr elf_header;
    if(file) {
        fread(&elf_header, 1, sizeof(elf_header), file);
        if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) == 0) {
            debugger->elf_header = elf_header;
        }
        fclose(file);
    }
}
// Only for PIE 
void read_load_address(debugger_t *debugger) {
    char file_location[128];
    sprintf(file_location, "/proc/%d/maps", debugger->child_pid);
    printf("%s", file_location);
    FILE* file = fopen(file_location, "rb");
    long long address;
    int c;
    if(file) {
        fscanf(file, "%llx", &address);
        debugger->load_address = address;
    }
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
    if(debugger->elf_header.e_type == ET_EXEC) {
        //No-pie
        breakpoint->addr = addr;
    } else {
        breakpoint->addr = addr + debugger->load_address;
    }
    breakpoint->word_before_bp = ptrace(PTRACE_PEEKDATA, debugger->child_pid,
            breakpoint->addr , 0);
    debugger->breakpoints[debugger->number_of_breakpoints++] = *breakpoint;

    enable_breakpoint(debugger->child_pid, breakpoint);
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
    read_load_address(debugger);

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
        if(strcmp(command, "reg") == 0) {
            char reg[4];
            printf("\nEnter the register name: ");
            scanf("%3s", reg);
            printf("\n[%s] %llx : ",reg, get_register_value(debugger, reg));
            continue;
            //dump((unsigned long) ptrace(PTRACE_PEEKTEXT, pid, registers.rip, 0)
        } else if(strcmp(command, "address_bp") == 0) {
            unsigned long long address;
            printf("\nEnter the address without \"0x\": ");
            scanf("%llx", &address);

            add_breakpoint(debugger, address);
            continue;
        } else if(strcmp(command, "function_bp") == 0) {
            printf("\nEnter the function name: ");
            char function_name[128];
            scanf("%127s", function_name);
            add_breakpoint(debugger, 
                process_cu(debugger->child_program_name, function_name));
            continue;
        } else if(strcmp(command, "disable_bp") == 0) {
            unsigned long long address;
            printf("\nEnter the address without \"0x\": ");
            scanf("%x", &address);
            breakpoint_t *bp = get_breakpoint(debugger, address);
            if(bp != NULL) {
                disable_breakpoint(debugger->child_pid, bp);
            }
            continue;
        } else if(strcmp(command, "exit") == 0) {
            exit(0);
        }

        if(WSTOPSIG(status) == SIGTRAP && WIFSTOPPED(status)) {
            if(strcmp(command, "step") == 0) {
                if(disable_last_bp) {
                    if(ptrace(PTRACE_SINGLESTEP, debugger->child_pid
                                , NULL, NULL) < 0) {
                        printf("\nERROR: ptrace() SINGLESTEP failure.");
                        continue;
                    }
                    waitpid(debugger->child_pid, &status, 0);
                    disable_last_bp = false;
                    enable_breakpoint(debugger->child_pid, bp);
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
                    enable_breakpoint(debugger->child_pid, bp);
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
                disable_breakpoint(debugger->child_pid, bp);
                disable_last_bp = true;
            } 

        }
    }
}