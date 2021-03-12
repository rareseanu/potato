#include <stdbool.h>

typedef struct breakpoint_t {
    unsigned long long addr;
    unsigned long long word_before_bp;
    unsigned number;
    bool enabled;
} breakpoint_t;



