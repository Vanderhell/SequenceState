#ifndef SEQUENCE_STATE_COMPOSE_H
#define SEQUENCE_STATE_COMPOSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t a[8];
    uint32_t b[8];
} ss_compose_state_t;

/* H: exact affine summary of an ordered sequence. */
int ss_compose_init(ss_compose_state_t *state);
int ss_compose_update(ss_compose_state_t *state, uint32_t symbol);
int ss_compose_combine(const ss_compose_state_t *left,
                       const ss_compose_state_t *right,
                       ss_compose_state_t *out);
int ss_compose_inverse(const ss_compose_state_t *state,
                       ss_compose_state_t *out);
int ss_compose_equal(const ss_compose_state_t *left,
                     const ss_compose_state_t *right);

#ifdef __cplusplus
}
#endif

#endif
