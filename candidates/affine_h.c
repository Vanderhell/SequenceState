#include "affine_h.h"

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

void ss_affine512_init(ss_affine512_t *state)
{
    for (uint32_t i = 0U; i < 8U; ++i) { state->a[i] = 1U; state->b[i] = 0U; }
}

void ss_affine512_update(ss_affine512_t *state, uint32_t symbol)
{
    for (uint32_t i = 0U; i < 8U; ++i) {
        const uint32_t multiplier = 2U * mix32(symbol + i * UINT32_C(0x9e3779b9)) + 1U;
        const uint32_t offset = mix32(symbol ^ (i * UINT32_C(0xa5a5a5a5)));
        state->b[i] = multiplier * state->b[i] + offset;
        state->a[i] = multiplier * state->a[i];
    }
}

/* combine(left,right) represents right o left. */
void ss_affine512_combine(const ss_affine512_t *left, const ss_affine512_t *right, ss_affine512_t *out)
{
    for (uint32_t i = 0U; i < 8U; ++i) {
        out->a[i] = right->a[i] * left->a[i];
        out->b[i] = right->a[i] * left->b[i] + right->b[i];
    }
}

void ss_affine512_inverse(const ss_affine512_t *state, ss_affine512_t *out)
{
    for (uint32_t i = 0U; i < 8U; ++i) {
        out->a[i] = inverse_odd32(state->a[i]);
        out->b[i] = 0U - out->a[i] * state->b[i];
    }
}

int ss_affine512_equal(const ss_affine512_t *a, const ss_affine512_t *b)
{
    uint32_t diff = 0U;
    for (uint32_t i = 0U; i < 8U; ++i) diff |= a->a[i] ^ b->a[i] ^ a->b[i] ^ b->b[i];
    return diff == 0U;
}
