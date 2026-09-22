#include <stdint.h>
#include <stdio.h>
#include "sequence_state/trace.h"
#include "sequence_state/compose.h"
#include "sequence_state/range.h"

static uint32_t next_u32(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static int fail_case(const char *name, uint32_t seed, uint32_t index)
{
    fprintf(stderr, "FAIL %s seed=%08x index=%u\n", name, seed, index);
    return 0;
}

int main(void)
{
    uint32_t seed = UINT32_C(0x51a7e); int ok = 1;
    for (uint32_t i = 0U; i < 1000000U && ok; ++i) {
        ss_trace_state_t state, before;
        ss_trace_init(&state);
        for (uint32_t j = 0U; j < (i & 15U); ++j) ss_trace_update(&state, next_u32(&seed));
        before = state;
        const uint32_t symbol = next_u32(&seed);
        ss_trace_update(&state, symbol); ss_trace_inverse_update(&state, symbol);
        if (!ss_trace_equal(&state, &before)) ok = fail_case("trace_roundtrip", seed, i);
    }
    for (uint32_t i = 0U; i < 100000U && ok; ++i) {
        ss_compose_state_t left, right, whole, combined, inverse, recovered;
        ss_compose_init(&left); ss_compose_init(&right); ss_compose_init(&whole);
        const uint32_t left_len = next_u32(&seed) & 31U;
        const uint32_t right_len = next_u32(&seed) & 31U;
        for (uint32_t j = 0U; j < left_len; ++j) { const uint32_t x = next_u32(&seed); ss_compose_update(&left, x); ss_compose_update(&whole, x); }
        for (uint32_t j = 0U; j < right_len; ++j) { const uint32_t x = next_u32(&seed); ss_compose_update(&right, x); ss_compose_update(&whole, x); }
        ss_compose_combine(&left, &right, &combined);
        ss_compose_inverse(&right, &inverse); ss_compose_combine(&whole, &inverse, &recovered);
        if (!ss_compose_equal(&whole, &combined) || !ss_compose_equal(&left, &recovered)) ok = fail_case("compose_identity", seed, i);
    }
    for (uint32_t i = 0U; i < 100000U && ok; ++i) {
        ss_range_state_t left, right, whole, combined, suffix;
        ss_range_init(&left); ss_range_init(&right); ss_range_init(&whole);
        const uint32_t left_len = next_u32(&seed) & 31U;
        const uint32_t right_len = next_u32(&seed) & 31U;
        for (uint32_t j = 0U; j < left_len; ++j) { const uint32_t x = next_u32(&seed); ss_range_update(&left, x); ss_range_update(&whole, x); }
        for (uint32_t j = 0U; j < right_len; ++j) { const uint32_t x = next_u32(&seed); ss_range_update(&right, x); ss_range_update(&whole, x); }
        ss_range_combine(&left, &right, &combined);
        if (ss_range_remove_prefix(&whole, &left, &suffix) != 1 || !ss_range_equal(&whole, &combined) || !ss_range_equal(&right, &suffix)) ok = fail_case("range_identity", seed, i);
    }
    puts(ok ? "PASS property stress" : "FAIL property stress");
    return ok ? 0 : 1;
}
