#ifndef SS_CANDIDATES_H
#define SS_CANDIDATES_H
#include "sequence_state.h"
typedef enum { SS_CANDIDATE_A, SS_CANDIDATE_B, SS_CANDIDATE_C, SS_CANDIDATE_D, SS_CANDIDATE_E, SS_CANDIDATE_F, SS_CANDIDATE_COUNT } ss_candidate_id;
const char *ss_candidate_name(ss_candidate_id id);
void ss_candidate_init(ss_candidate_id id, ss_state256_t *state);
void ss_candidate_update(ss_candidate_id id, ss_state256_t *state, uint32_t symbol);
void ss_candidate_c_inverse(ss_state256_t *state, uint32_t symbol);
#endif
