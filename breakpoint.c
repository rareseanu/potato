#include "breakpoint.h"

void enable_breakpoint(breakpoint_t *breakpoint) {
    breakpoint->enabled = true;
}

void disable_breakpoint(breakpoint_t *breakpoint) {
    breakpoint->enabled = false;
}
