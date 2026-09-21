#ifndef SS_AFFINE_H_H
#define SS_AFFINE_H_H

#include <stdint.h>

typedef struct { uint32_t a[8]; uint32_t b[8]; } ss_affine512_t;

void ss_affine512_init(ss_affine512_t *state);
void ss_affine512_update(ss_affine512_t *state, uint32_t symbol);
void ss_affine512_combine(const ss_affine512_t *left, const ss_affine512_t *right, ss_affine512_t *out);
void ss_affine512_inverse(const ss_affine512_t *state, ss_affine512_t *out);
int ss_affine512_equal(const ss_affine512_t *a, const ss_affine512_t *b);

#endif
