#ifndef SEQUENCE_STATE_H
#define SEQUENCE_STATE_H

#include <stdint.h>

typedef struct { uint32_t lane[8]; } ss_state256_t;

void ss256_init(ss_state256_t *state);
void ss256_update(ss_state256_t *state, uint32_t symbol);
int ss256_equal(const ss_state256_t *a, const ss_state256_t *b);
uint32_t ss256_hamming_distance(const ss_state256_t *a, const ss_state256_t *b);

/* Reference candidate: deterministic ARX transform, not a cryptographic hash. */
uint32_t ss_rotl32(uint32_t value, uint32_t amount);

#endif
