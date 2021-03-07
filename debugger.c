#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("ERROR: Specify the program name.\n");
        return -1;
    }
    
    char* programName = argv[1];
    char* programArgs[] = { argv[1], (char*) 0 };

    // fork() - create an exact copy (child process)  of this calling 
    // process which runs concurrently with the parent. 
    pid_t pid = fork();

    if(pid < 0) {
        printf("ERROR: Unsuccessful fork() call.\n");
        return -1;
    } else if(pid == 0) {
       // Child process. 
        
        // ptrace() - observe/control the execution of another process
        ptrace(PTRACE_TRACEME, pid, NULL, NULL);
        execve(programName, programArgs, NULL);
    } else {
        // Parent process.
        printf("Debug on pid: %d\n", pid);
        int status;
        
        // waitpid() - wait for state changes in the given child process
        waitpid(pid, &status, 0);
        
        char  command[50];
        const char* exitCommand = "exit";
        
        printf("%d", status);
        do {
            printf("\ndebugger > ");
            scanf("%s", command);
            if(strcmp(command, "next") == 0) {
                ptrace(PTRACE_CONT, pid, NULL, NULL);
                waitpid(pid, &status, 0);
                printf("%d", status);
            }
        } while(strcmp(command, exitCommand) != 0);        
    }

}
