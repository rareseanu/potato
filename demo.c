#include "debugger.h"

void dump(long data) {
    for(int i = 0; i < 64; i += 8) {
        printf("%02x ", ((unsigned long) data >> i) & 0xff);
    }
}


int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("ERROR: Specify the program name.\n");
        return -1;
    }
    
    char* programArgs[] = { argv[1], (char*) 0 };
    debugger_t debugger;
    debugger.child_program_name = argv[1];
     
    // fork() - create an exact copy (child process)  of this calling 
    // process which runs concurrently with the parent. 
    pid_t pid = fork();
    if(pid < 0) {
        printf("ERROR: Unsuccessful fork() call.\n");
        return -1;
    } else if(pid == 0) {
        // Child process. 
        initiate_child_trace(&debugger, programArgs);    
    } else {
        // Parent (debugger) process.
        debugger.child_pid = pid;    
        run_debugger(&debugger); 
    }
}