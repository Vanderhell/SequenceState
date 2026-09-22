#include <stddef.h>
#include "sequence_state/range.h"

static uint32_t mix32(uint32_t x)
{
    x ^= x >> 16U; x *= UINT32_C(0x7feb352d); x ^= x >> 15U;
    x *= UINT32_C(0x846ca68b); return x ^ (x >> 16U);
}

static uint64_t power_r(uint64_t exponent)
{
    uint64_t base = UINT64_C(0x9e3779b185ebca87), result = 1U;
    while (exponent != 0U) {
        if ((exponent & 1U) != 0U) result *= base;
        base *= base; exponent >>= 1U;
    }
    return result;
}

static uint64_t value_for(uint32_t symbol)
{
    return (uint64_t)mix32(symbol) * UINT64_C(0x100000001b3);
}

int ss_range_init(ss_range_state_t *state)
{
    if (state == NULL) return -1;
    state->length = 0U; state->polynomial = 0U; state->sum = 0U; state->moment = 0U;
    return 0;
}

int ss_range_update(ss_range_state_t *state, uint32_t symbol)
{
    if (state == NULL) return -1;
    const uint64_t value = value_for(symbol);
    state->polynomial = state->polynomial * UINT64_C(0x9e3779b185ebca87) + value;
    state->moment += state->length * value;
    state->sum += value;
    ++state->length;
    return 0;
}

int ss_range_combine(const ss_range_state_t *left,
                     const ss_range_state_t *right,
                     ss_range_state_t *out)
{
    if (left == NULL || right == NULL || out == NULL) return -1;
    if (out != left && out != right) {
        out->length = left->length + right->length;
        out->polynomial = left->polynomial * power_r(right->length) + right->polynomial;
        out->sum = left->sum + right->sum;
        out->moment = left->moment + right->moment + left->length * right->sum;
        return 0;
    }
    const uint64_t left_length = left->length, right_length = right->length;
    const uint64_t left_polynomial = left->polynomial, right_polynomial = right->polynomial;
    const uint64_t left_sum = left->sum, right_sum = right->sum, left_moment = left->moment, right_moment = right->moment;
    out->length = left_length + right_length;
    out->polynomial = left_polynomial * power_r(right_length) + right_polynomial;
    out->sum = left_sum + right_sum;
    out->moment = left_moment + right_moment + left_length * right_sum;
    return 0;
}

int ss_range_remove_prefix(const ss_range_state_t *total,
                           const ss_range_state_t *prefix,
                           ss_range_state_t *out)
{
    if (total == NULL || prefix == NULL || out == NULL || prefix->length > total->length) return 0;
    if (out != total && out != prefix) {
        const uint64_t right_len = total->length - prefix->length;
        const uint64_t right_sum = total->sum - prefix->sum;
        out->length = right_len;
        out->polynomial = total->polynomial - prefix->polynomial * power_r(right_len);
        out->sum = right_sum;
        out->moment = total->moment - prefix->moment - prefix->length * right_sum;
        return 1;
    }
    const uint64_t total_length = total->length, prefix_length = prefix->length;
    const uint64_t total_polynomial = total->polynomial, prefix_polynomial = prefix->polynomial;
    const uint64_t total_sum = total->sum, prefix_sum = prefix->sum;
    const uint64_t total_moment = total->moment, prefix_moment = prefix->moment;
    const uint64_t right_len = total_length - prefix_length;
    const uint64_t right_sum = total_sum - prefix_sum;
    out->length = right_len;
    out->polynomial = total_polynomial - prefix_polynomial * power_r(right_len);
    out->sum = right_sum;
    out->moment = total_moment - prefix_moment - prefix_length * right_sum;
    return 1;
}

int ss_range_equal(const ss_range_state_t *left,
                   const ss_range_state_t *right)
{
    if (left == NULL || right == NULL) return 0;
    return left->length == right->length && left->polynomial == right->polynomial
        && left->sum == right->sum && left->moment == right->moment;
}
