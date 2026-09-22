#include <stdio.h>
#include "sequence_state/trace.h"

int main(void)
{
    ss_trace_state_t state, before;
    if (ss_trace_init(&state) != 0) return 1;
    before = state;
    if (ss_trace_update(&state, 0x10U) != 0) return 1;
    if (ss_trace_inverse_update(&state, 0x10U) != 0) return 1;
    printf("rollback=%s\n", ss_trace_equal(&state, &before) ? "exact" : "failed");
    return ss_trace_equal(&state, &before) ? 0 : 1;
}
