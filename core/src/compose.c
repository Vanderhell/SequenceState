#include <stddef.h>
#include "sequence_state/compose.h"

static uint32_t mix32(uint32_t x)
{
    x ^= x >> 16U; x *= UINT32_C(0x7feb352d); x ^= x >> 15U;
    x *= UINT32_C(0x846ca68b); return x ^ (x >> 16U);
}

static uint32_t inverse_odd32(uint32_t x)
{
    uint32_t y = x;
    for (uint32_t i = 0U; i < 5U; ++i) y *= 2U - x * y;
    return y;
}

int ss_compose_init(ss_compose_state_t *state)
{
    if (state == NULL) return -1;
    for (uint32_t i = 0U; i < 8U; ++i) { state->a[i] = 1U; state->b[i] = 0U; }
    return 0;
}

int ss_compose_update(ss_compose_state_t *state, uint32_t symbol)
{
    if (state == NULL) return -1;
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t multiplier = 2U * mix32(symbol + i * UINT32_C(0x9e3779b9)) + 1U;
        const uint32_t offset = mix32(symbol ^ (i * UINT32_C(0xa5a5a5a5)));
        state->b[i] = multiplier * state->b[i] + offset;
        state->a[i] = multiplier * state->a[i];
    }
    return 0;
}

int ss_compose_combine(const ss_compose_state_t *left,
                       const ss_compose_state_t *right,
                       ss_compose_state_t *out)
{
    if (left == NULL || right == NULL || out == NULL) return -1;
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t left_a = left->a[i], left_b = left->b[i];
        const uint32_t right_a = right->a[i], right_b = right->b[i];
        out->a[i] = right_a * left_a;
        out->b[i] = right_a * left_b + right_b;
    }
    return 0;
}

int ss_compose_inverse(const ss_compose_state_t *state, ss_compose_state_t *out)
{
    if (state == NULL || out == NULL) return -1;
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t input_a = state->a[i], input_b = state->b[i];
        out->a[i] = inverse_odd32(input_a);
        out->b[i] = 0U - out->a[i] * input_b;
    }
    return 0;
}

int ss_compose_equal(const ss_compose_state_t *left,
                     const ss_compose_state_t *right)
{
    if (left == NULL || right == NULL) return 0;
    uint32_t diff = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) diff |= left->a[i] ^ right->a[i] ^ left->b[i] ^ right->b[i];
    return diff == 0U;
}
