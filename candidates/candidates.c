#include "candidates.h"

static uint32_t mix(uint32_t x) { x ^= x >> 16U; x *= UINT32_C(0x7feb352d); x ^= x >> 15U; x *= UINT32_C(0x846ca68b); return x ^ (x >> 16U); }
const char *ss_candidate_name(ss_candidate_id id)
{
    static const char *names[SS_CANDIDATE_COUNT] = {"A-arx", "B-cross-multiply", "C-reversible", "D-invariant", "E-permutation", "F-composable"};
    return id < SS_CANDIDATE_COUNT ? names[id] : "unknown";
}
void ss_candidate_init(ss_candidate_id id, ss_state256_t *state)
{
    ss256_init(state);
    state->lane[0] ^= UINT32_C(0x10001) * ((uint32_t)id + 1U);
}
void ss_candidate_update(ss_candidate_id id, ss_state256_t *s, uint32_t x)
{
    const uint32_t k = mix(x + UINT32_C(0x9e3779b9) * ((uint32_t)id + 1U));
    if (id == SS_CANDIDATE_A) { for (uint32_t i=0U;i<8U;++i) s->lane[i]=ss_rotl32(s->lane[i]+k+(i*UINT32_C(0x632be59b)),(x+i*3U)&31U)^s->lane[(i+3U)&7U]; }
    else if (id == SS_CANDIDATE_B) { for (uint32_t i=0U;i<8U;++i) s->lane[i]=(s->lane[i]*((k|1U)+i*2U)+ss_rotl32(s->lane[(i+1U)&7U],11U))^k; }
    else if (id == SS_CANDIDATE_C) { for (uint32_t i=0U;i<8U;++i) s->lane[i]=ss_rotl32(s->lane[i]^k,(i*7U+x)&31U)*UINT32_C(0x9e3779b1); uint32_t t=s->lane[0]; s->lane[0]=s->lane[3];s->lane[3]=s->lane[6];s->lane[6]=t; }
    else if (id == SS_CANDIDATE_D) { for (uint32_t i=0U;i<8U;++i) s->lane[i] ^= ss_rotl32(k+i*UINT32_C(0x45d9f3b),i*4U); }
    else if (id == SS_CANDIDATE_E) { const uint32_t p=(x^k)&7U; for(uint32_t i=0U;i<8U;++i)s->lane[i]=ss_rotl32(s->lane[(i+p)&7U]+k,i+p); }
    else { for (uint32_t i=0U;i<8U;++i)s->lane[i]+=ss_rotl32(k^s->lane[(i+1U)&7U],i*3U); }
}
