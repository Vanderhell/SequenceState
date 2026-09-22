#ifndef SEQUENCE_STATE_TRACE_H
#define SEQUENCE_STATE_TRACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t lane[8];
} ss_trace_state_t;

/* Returns 0 on success, -1 when state is NULL. */
int ss_trace_init(ss_trace_state_t *state);
int ss_trace_update(ss_trace_state_t *state, uint32_t symbol);
int ss_trace_inverse_update(ss_trace_state_t *state, uint32_t symbol);

/* Equality returns 0 for invalid pointers; a valid state is never NULL. */
int ss_trace_equal(const ss_trace_state_t *left, const ss_trace_state_t *right);
/* UINT32_MAX denotes an invalid pointer; otherwise the Hamming distance is returned. */
uint32_t ss_trace_hamming_distance(const ss_trace_state_t *left,
                                   const ss_trace_state_t *right);

#ifdef __cplusplus
}
#endif

#endif
