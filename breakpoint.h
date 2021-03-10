#include <stdbool.h>

typedef struct breakpoint_t {
    unsigned long long addr;
    unsigned long long word_before_bp;
    bool enabled;
} breakpoint_t;

void enable_breakpoint(breakpoint_t *breakpoint);
void disable_breakpoint(breakpoint_t *breakpoint);
