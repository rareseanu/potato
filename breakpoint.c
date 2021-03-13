#include "breakpoint.h"

int disable_breakpoint(pid_t pid, breakpoint_t *breakpoint) {

    struct user_regs_struct registers;
    if(ptrace(PTRACE_GETREGS, pid, NULL, &registers) <0) {
        printf("\nERROR: ptrace GETREGS failure.");   
        return -1;
    }

    //The instruction pointer now points to the instruction after
    //the trap so we need to change it to point to the 
    //original instruction that we will replace back.
    registers.rip = breakpoint->addr;
    if(ptrace(PTRACE_SETREGS, pid, NULL, &registers) < 0) {
        printf("\nERROR: ptrace SETREGS failure.");
        return -1;
    }

    printf("\n\tBefore breakpoint disable: %016x", ptrace(PTRACE_PEEKDATA, 
                pid, breakpoint->addr, NULL) );

    unsigned long long data_with_int3 = ptrace(PTRACE_PEEKTEXT
            , pid, breakpoint->addr, 0); 
    //Apply the mask on the so it takes all but the first byte that contains
    //INT 3 (0xCC) and replace it with the first byte of the original instruction.
    unsigned long long constructed_back = (data_with_int3 & ~0xff) 
        | (breakpoint->word_before_bp & 0xff);

    if(ptrace(PTRACE_POKETEXT, pid, breakpoint->addr,
                constructed_back) < 0) {
        printf("\nERROR: ptrace POKEDATA\n");
        return -1;
    }
    printf("\n\tAfter breakpoint disable: %016x", ptrace(PTRACE_PEEKDATA, 
                pid, breakpoint->addr, NULL));
    return 0;
}

int enable_breakpoint(pid_t pid, breakpoint_t *breakpoint) {

    //Save the word so we can run the original instruction after 
    //disabling the breakpoint.
    breakpoint->word_before_bp = ptrace(PTRACE_PEEKDATA, pid,
            breakpoint->addr, 0);
    printf("\n\tBefore breakpoint enable: %016x", ptrace(PTRACE_PEEKDATA, 
                pid, breakpoint->addr, NULL));  

    //0xcc - INT 3 opcode (software breakpoint)
    //apply the mask on the word so it takes all but the first byte
    //replace the first byte with INT 3 opcode  
    if(ptrace(PTRACE_POKEDATA, pid, breakpoint->addr,
                ((breakpoint->word_before_bp & ~0xff) | 0xcc)) < 0) {
        printf("\n%d", pid);
        printf("\nERROR: ptrace() POKEDATA failure: %s\n", strerror(errno));
        return -1;
    }

    printf("\n\tAfter breakpoint enable: %016x\n", ptrace(PTRACE_PEEKDATA, 
                pid, breakpoint->addr, NULL) );
    return 0;  
}

