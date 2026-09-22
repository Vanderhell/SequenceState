#ifndef SEQUENCE_STATE_RANGE_H
#define SEQUENCE_STATE_RANGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t length;
    uint64_t polynomial;
    uint64_t sum;
    uint64_t moment;
} ss_range_state_t;

/* H2: exact-composable summary with prefix removal and window support. */
int ss_range_init(ss_range_state_t *state);
int ss_range_update(ss_range_state_t *state, uint32_t symbol);
int ss_range_combine(const ss_range_state_t *left,
                     const ss_range_state_t *right,
                     ss_range_state_t *out);
/* Returns 0 when prefix is longer than total or a pointer is invalid. */
int ss_range_remove_prefix(const ss_range_state_t *total,
                           const ss_range_state_t *prefix,
                           ss_range_state_t *out);
int ss_range_equal(const ss_range_state_t *left,
                   const ss_range_state_t *right);

#ifdef __cplusplus
}
#endif

#endif
