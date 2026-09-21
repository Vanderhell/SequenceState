#ifndef SS_H2_H
#define SS_H2_H
#include <stdint.h>

typedef struct { uint64_t length; uint64_t polynomial; uint64_t sum; uint64_t moment; } ss_h2_state_t;
void ss_h2_init(ss_h2_state_t *state);
void ss_h2_update(ss_h2_state_t *state, uint32_t symbol);
void ss_h2_combine(const ss_h2_state_t *left, const ss_h2_state_t *right, ss_h2_state_t *out);
int ss_h2_remove_prefix(const ss_h2_state_t *total, const ss_h2_state_t *prefix, ss_h2_state_t *out);
int ss_h2_equal(const ss_h2_state_t *a, const ss_h2_state_t *b);

#endif
