#include <stdint.h>
#include <stdio.h>
#include "sequence_state/trace.h"
#include "sequence_state/compose.h"
#include "sequence_state/range.h"

static uint32_t rng(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static int check(int condition, const char *name)
{
    if (!condition) fprintf(stderr, "FAIL %s\n", name);
    return condition;
}

static int test_trace(void)
{
    int ok = 1;
    ss_trace_state_t state, before;
    ok &= check(ss_trace_init(&state) == 0, "trace init");
    for (uint32_t i = 0U; i < 100000U; ++i) {
        before = state;
        const uint32_t symbol = i * UINT32_C(2654435761);
        ok &= check(ss_trace_update(&state, symbol) == 0, "trace update");
        ok &= check(ss_trace_inverse_update(&state, symbol) == 0 && ss_trace_equal(&state, &before), "trace inverse");
    }
    ok &= check(ss_trace_init(NULL) == -1, "trace null init");
    ok &= check(ss_trace_update(NULL, 0U) == -1, "trace null update");
    ok &= check(ss_trace_inverse_update(NULL, 0U) == -1, "trace null inverse");
    ok &= check(ss_trace_equal(NULL, &state) == 0, "trace null equal");
    ok &= check(ss_trace_hamming_distance(NULL, &state) == UINT32_MAX, "trace null distance");
    ss_trace_init(&state); ss_trace_update(&state, 0U);
    ok &= check(state.lane[0] == UINT32_C(0x77f3f57d) && state.lane[7] == UINT32_C(0xdfcc5644), "trace frozen vector");
    return ok;
}

static int test_compose(void)
{
    int ok = 1;
    ss_compose_state_t left, right, whole, combined, nested, inverse, identity;
    ok &= check(ss_compose_init(&left) == 0 && ss_compose_init(&right) == 0 && ss_compose_init(&whole) == 0, "compose init");
    for (uint32_t i = 0U; i < 4U; ++i) { ss_compose_update(&left, i); ss_compose_update(&whole, i); }
    for (uint32_t i = 4U; i < 8U; ++i) { ss_compose_update(&right, i); ss_compose_update(&whole, i); }
    ok &= check(ss_compose_combine(&left, &right, &combined) == 0 && ss_compose_equal(&whole, &combined), "compose concatenate");
    ss_compose_state_t left_copy = left;
    ok &= check(ss_compose_combine(&left_copy, &right, &left_copy) == 0 && ss_compose_equal(&left_copy, &combined), "compose left alias");
    ss_compose_state_t right_copy = right;
    ok &= check(ss_compose_combine(&left, &right_copy, &right_copy) == 0 && ss_compose_equal(&right_copy, &combined), "compose right alias");
    ss_compose_init(&left); ss_compose_init(&right); ss_compose_init(&whole);
    for (uint32_t i = 0U; i < 2U; ++i) { ss_compose_update(&left, i); ss_compose_update(&whole, i); }
    for (uint32_t i = 2U; i < 4U; ++i) { ss_compose_update(&right, i); ss_compose_update(&whole, i); }
    ss_compose_state_t third, ab, bc;
    ss_compose_init(&third); ss_compose_update(&third, 4U); ss_compose_update(&whole, 4U);
    ss_compose_combine(&left, &right, &ab); ss_compose_combine(&ab, &third, &nested);
    ss_compose_combine(&right, &third, &bc); ss_compose_combine(&left, &bc, &combined);
    ok &= check(ss_compose_equal(&nested, &combined), "compose associative");
    ok &= check(ss_compose_inverse(&right, &inverse) == 0, "compose inverse operation");
    ok &= check(ss_compose_init(&identity) == 0 && ss_compose_combine(&left, &identity, &combined) == 0 && ss_compose_equal(&left, &combined), "compose identity");
    ok &= check(ss_compose_combine(NULL, &right, &combined) == -1, "compose null combine");
    ss_compose_init(&whole); ss_compose_update(&whole, 0U);
    ok &= check(whole.a[0] == UINT32_C(0x00000001) && whole.b[0] == UINT32_C(0x00000000)
                && whole.a[1] == UINT32_C(0x03f9caa5) && whole.b[1] == UINT32_C(0x80c5e7d3), "compose frozen vector");
    return ok;
}

static int test_range(void)
{
    int ok = 1;
    ss_range_state_t left, right, whole, combined, suffix, alias;
    ok &= check(ss_range_init(&left) == 0 && ss_range_init(&right) == 0 && ss_range_init(&whole) == 0, "range init");
    for (uint32_t i = 0U; i < 4U; ++i) { ss_range_update(&left, i); ss_range_update(&whole, i); }
    for (uint32_t i = 4U; i < 8U; ++i) { ss_range_update(&right, i); ss_range_update(&whole, i); }
    ok &= check(ss_range_combine(&left, &right, &combined) == 0 && ss_range_equal(&whole, &combined), "range concatenate");
    ok &= check(ss_range_remove_prefix(&whole, &left, &suffix) == 1 && ss_range_equal(&suffix, &right), "range prefix removal");
    alias = whole;
    ok &= check(ss_range_remove_prefix(&alias, &left, &alias) == 1 && ss_range_equal(&alias, &right), "range output alias");
    ss_range_state_t left_copy = left;
    ok &= check(ss_range_combine(&left_copy, &right, &left_copy) == 0 && ss_range_equal(&left_copy, &combined), "range left alias");
    ss_range_state_t too_long = combined;
    ss_range_update(&too_long, 8U);
    ok &= check(ss_range_remove_prefix(&whole, &too_long, &suffix) == 0, "range invalid prefix");
    ok &= check(ss_range_combine(NULL, &right, &combined) == -1, "range null combine");
    ss_range_init(&whole); ss_range_update(&whole, 0U);
    ok &= check(whole.length == 1U && whole.polynomial == UINT64_C(0x0000000000000000), "range frozen vector");
    return ok;
}

static int test_determinism(void)
{
    ss_trace_state_t a, b;
    uint32_t seed_a = UINT32_C(123456789), seed_b = UINT32_C(123456789);
    int ok = ss_trace_init(&a) == 0 && ss_trace_init(&b) == 0;
    for (uint32_t i = 0U; i < 10000U; ++i) { ss_trace_update(&a, rng(&seed_a)); ss_trace_update(&b, rng(&seed_b)); }
    return check(ok && ss_trace_equal(&a, &b), "trace determinism");
}

int main(void)
{
    const int ok = test_trace() && test_compose() && test_range() && test_determinism();
    printf("%s\n", ok ? "PASS production properties" : "FAIL production properties");
    return ok ? 0 : 1;
}
