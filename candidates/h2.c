#include "h2.h"

static uint32_t mix32_h2(uint32_t x)
{
    x ^= x >> 16U; x *= UINT32_C(0x7feb352d); x ^= x >> 15U;
    x *= UINT32_C(0x846ca68b); return x ^ (x >> 16U);
}

static uint64_t power_r(uint64_t exponent)
{
    uint64_t base = UINT64_C(0x9e3779b185ebca87), result = 1U;
    while (exponent != 0U) { if ((exponent & 1U) != 0U) result *= base; base *= base; exponent >>= 1U; }
    return result;
}

static uint64_t value_for(uint32_t symbol)
{
    return (uint64_t)mix32_h2(symbol) * UINT64_C(0x100000001b3);
}

void ss_h2_init(ss_h2_state_t *state) { state->length=0U; state->polynomial=0U; state->sum=0U; state->moment=0U; }
void ss_h2_update(ss_h2_state_t *state, uint32_t symbol)
{
    const uint64_t value=value_for(symbol); state->polynomial=state->polynomial*UINT64_C(0x9e3779b185ebca87)+value;
    state->moment += state->length*value; state->sum += value; ++state->length;
}
void ss_h2_combine(const ss_h2_state_t *left,const ss_h2_state_t *right,ss_h2_state_t *out)
{
    out->length=left->length+right->length; out->polynomial=left->polynomial*power_r(right->length)+right->polynomial;
    out->sum=left->sum+right->sum; out->moment=left->moment+right->moment+left->length*right->sum;
}
int ss_h2_remove_prefix(const ss_h2_state_t *total,const ss_h2_state_t *prefix,ss_h2_state_t *out)
{
    if (prefix->length>total->length) return 0;
    const uint64_t right_len=total->length-prefix->length; const uint64_t right_sum=total->sum-prefix->sum;
    out->length=right_len; out->polynomial=total->polynomial-prefix->polynomial*power_r(right_len); out->sum=right_sum; out->moment=total->moment-prefix->moment-prefix->length*right_sum; return 1;
}
int ss_h2_equal(const ss_h2_state_t *a,const ss_h2_state_t *b)
{
    return a->length==b->length&&a->polynomial==b->polynomial&&a->sum==b->sum&&a->moment==b->moment;
}
