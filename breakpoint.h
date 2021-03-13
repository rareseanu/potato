#include <stdbool.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

typedef struct breakpoint_t {
    unsigned long long addr;
    unsigned long long word_before_bp;
    unsigned number;
} breakpoint_t;

int disable_breakpoint(pid_t pid, breakpoint_t *breakpoint);
int enable_breakpoint(pid_t pid, breakpoint_t *breakpoint);

