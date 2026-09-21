#include "baselines.h"
void ss_xor_update(ss_word_state *s,uint32_t x){s->value^=x;}
void ss_sum_update(ss_word_state *s,uint32_t x){s->value+=x;}
void ss_fnv_update(ss_word_state *s,uint32_t x){s->value^=x;s->value*=UINT32_C(16777619);}
void ss_crc_update(ss_word_state *s,uint32_t x){s->value^=x;for(uint32_t i=0U;i<32U;++i)s->value=(s->value&1U)!=0U?(s->value>>1U)^UINT32_C(0xedb88320):s->value>>1U;}
