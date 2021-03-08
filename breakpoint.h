#include <stdbool.h>

typedef struct breakpoint_t {
    unsigned long addr;
    bool enabled;
} breakpoint_t;

void enable_breakpoint(breakpoint_t *breakpoint);
void disable_breakpoint(breakpoint_t *breakpoint);
