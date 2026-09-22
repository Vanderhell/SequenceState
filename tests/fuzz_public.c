#include <stddef.h>
#include <stdint.h>
#include "sequence_state/trace.h"
#include "sequence_state/compose.h"
#include "sequence_state/range.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ss_trace_state_t trace, trace_before;
    ss_compose_state_t compose, compose_block, compose_out;
    ss_range_state_t range, range_block, range_out;
    ss_trace_init(&trace); ss_compose_init(&compose); ss_compose_init(&compose_block);
    ss_range_init(&range); ss_range_init(&range_block);
    for (size_t i = 0U; i < size; ++i) {
        const uint32_t symbol = (uint32_t)data[i] | ((uint32_t)(i & 0xffU) << 8U);
        trace_before = trace;
        ss_trace_update(&trace, symbol);
        ss_trace_inverse_update(&trace, symbol);
        if (!ss_trace_equal(&trace, &trace_before)) return 1;
        ss_compose_update(&compose, symbol); ss_compose_update(&compose_block, symbol ^ UINT32_C(0x9e3779b9));
        ss_range_update(&range, symbol); ss_range_update(&range_block, symbol ^ UINT32_C(0x9e3779b9));
    }
    ss_compose_combine(&compose, &compose_block, &compose_out);
    ss_range_combine(&range, &range_block, &range_out);
    (void)ss_compose_equal(&compose_out, &compose);
    (void)ss_range_equal(&range_out, &range);
    return 0;
}
