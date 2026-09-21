#ifndef SS_BASELINES_H
#define SS_BASELINES_H
#include <stdint.h>
typedef struct { uint32_t value; } ss_word_state;
void ss_xor_update(ss_word_state *s, uint32_t x);
void ss_sum_update(ss_word_state *s, uint32_t x);
void ss_fnv_update(ss_word_state *s, uint32_t x);
void ss_crc_update(ss_word_state *s, uint32_t x);
#endif
