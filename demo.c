#include "debugger.h"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("ERROR: Specify the program name.\n");
        return -1;
    }
    
    char* programArgs[] = { argv[1], (char*) 0 };
    debugger_t debugger;
    initialize_debugger(0, &debugger, argv[1]);
    //debugger.child_program_name = argv[1];
    printf("\n\n\n %s", debugger.child_program_name); 
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
