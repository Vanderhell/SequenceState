#include <stddef.h>
#include "sequence_state/trace.h"

static uint32_t rotl32(uint32_t value, uint32_t amount)
{
    const uint32_t shift = amount & 31U;
    if (shift == 0U) return value;
    return (value << shift) | (value >> (32U - shift));
}

static uint32_t mix(uint32_t x)
{
    x ^= x >> 16U;
    x *= UINT32_C(0x7feb352d);
    x ^= x >> 15U;
    x *= UINT32_C(0x846ca68b);
    return x ^ (x >> 16U);
}

int ss_trace_init(ss_trace_state_t *state)
{
    if (state == NULL) return -1;
    state->lane[0] = UINT32_C(0x243f6a88); state->lane[1] = UINT32_C(0x85a308d3);
    state->lane[2] = UINT32_C(0x13198a2e); state->lane[3] = UINT32_C(0x03707344);
    state->lane[4] = UINT32_C(0xa4093822); state->lane[5] = UINT32_C(0x299f31d0);
    state->lane[6] = UINT32_C(0x082efa98); state->lane[7] = UINT32_C(0xec4e6c89);
    state->lane[0] ^= UINT32_C(0x10001) * 3U;
    return 0;
}

int ss_trace_update(ss_trace_state_t *state, uint32_t symbol)
{
    if (state == NULL) return -1;
    const uint32_t k = mix(symbol + UINT32_C(0x9e3779b9) * 3U);
    for (uint32_t i = 0U; i < 8U; ++i) {
        state->lane[i] = rotl32(state->lane[i] ^ k, (i * 7U + symbol) & 31U)
                       * UINT32_C(0x9e3779b1);
    }
    const uint32_t t = state->lane[0];
    state->lane[0] = state->lane[3]; state->lane[3] = state->lane[6]; state->lane[6] = t;
    return 0;
}

int ss_trace_inverse_update(ss_trace_state_t *state, uint32_t symbol)
{
    if (state == NULL) return -1;
    const uint32_t k = mix(symbol + UINT32_C(0x9e3779b9) * 3U);
    const uint32_t inverse_multiplier = UINT32_C(0x0e8b2f51);
    uint32_t transformed[8];
    for (uint32_t i = 0U; i < 8U; ++i) transformed[i] = state->lane[i];
    transformed[0] = state->lane[6]; transformed[3] = state->lane[0]; transformed[6] = state->lane[3];
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t shift = (i * 7U + symbol) & 31U;
        state->lane[i] = rotl32(transformed[i] * inverse_multiplier, (32U - shift) & 31U) ^ k;
    }
    return 0;
}

int ss_trace_equal(const ss_trace_state_t *left, const ss_trace_state_t *right)
{
    if (left == NULL || right == NULL) return 0;
    uint32_t diff = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) diff |= left->lane[i] ^ right->lane[i];
    return diff == 0U;
}

uint32_t ss_trace_hamming_distance(const ss_trace_state_t *left,
                                   const ss_trace_state_t *right)
{
    if (left == NULL || right == NULL) return UINT32_MAX;
    uint32_t result = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) {
        uint32_t bits = left->lane[i] ^ right->lane[i];
        while (bits != 0U) { bits &= bits - 1U; ++result; }
    }
    return result;
}
