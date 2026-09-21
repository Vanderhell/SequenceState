#include "sequence_state.h"

uint32_t ss_rotl32(uint32_t value, uint32_t amount)
{
    const uint32_t shift = amount & 31U;
    return (value << shift) | (value >> ((32U - shift) & 31U));
}

void ss256_init(ss_state256_t *state)
{
    state->lane[0] = UINT32_C(0x243f6a88); state->lane[1] = UINT32_C(0x85a308d3);
    state->lane[2] = UINT32_C(0x13198a2e); state->lane[3] = UINT32_C(0x03707344);
    state->lane[4] = UINT32_C(0xa4093822); state->lane[5] = UINT32_C(0x299f31d0);
    state->lane[6] = UINT32_C(0x082efa98); state->lane[7] = UINT32_C(0xec4e6c89);
}

void ss256_update(ss_state256_t *state, uint32_t symbol)
{
    uint32_t carry = symbol ^ UINT32_C(0x9e3779b9);
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t next = state->lane[(i + 1U) & 7U];
        const uint32_t mix = carry + UINT32_C(0x7f4a7c15) * (i + 1U);
        state->lane[i] = ss_rotl32(state->lane[i] ^ mix, (i * 5U + symbol) & 31U) + next;
        carry = state->lane[i] ^ ss_rotl32(carry, 7U);
    }
}

int ss256_equal(const ss_state256_t *a, const ss_state256_t *b)
{
    uint32_t diff = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) diff |= a->lane[i] ^ b->lane[i];
    return diff == 0U;
}

uint32_t ss256_hamming_distance(const ss_state256_t *a, const ss_state256_t *b)
{
    uint32_t result = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) {
        uint32_t bits = a->lane[i] ^ b->lane[i];
        while (bits != 0U) { bits &= bits - 1U; ++result; }
    }
    return result;
}
