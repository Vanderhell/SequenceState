#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sequence_state.h"
#include "candidates.h"
#include "baselines.h"
#include "affine_h.h"

static uint32_t rng(uint32_t *s){*s=*s*UINT32_C(1664525)+UINT32_C(1013904223);return *s;}
static int check(int condition,const char *name){if(!condition){fprintf(stderr,"FAIL %s\n",name);return 0;}return 1;}
int main(void)
{
    int ok=1; ss_state256_t a,b,c; uint32_t seed=UINT32_C(123456789);
    ss256_init(&a); b=a; for(uint32_t i=0U;i<10000U;++i)ss256_update(&a,rng(&seed)); seed=UINT32_C(123456789);for(uint32_t i=0U;i<10000U;++i)ss256_update(&b,rng(&seed));
    ok&=check(ss256_equal(&a,&b),"determinism"); ss256_init(&c);ss256_update(&c,1U);ok&=check(!ss256_equal(&c,&a),"nontrivial update");
    for(int id=0;id<SS_CANDIDATE_COUNT;++id){ss_candidate_init((ss_candidate_id)id,&a);ss_candidate_init((ss_candidate_id)id,&b);for(uint32_t i=0U;i<256U;++i){uint32_t x=rng(&seed);ss_candidate_update((ss_candidate_id)id,&a,x);ss_candidate_update((ss_candidate_id)id,&b,x);}ok&=check(ss256_equal(&a,&b),ss_candidate_name((ss_candidate_id)id));}
    ss_word_state x={0U},y={0U},p={1U}; uint64_t crc64=0U;for(uint32_t i=0U;i<100U;++i){ss_xor_update(&x,i);ss_sum_update(&y,i);ss_poly_update(&p,i);ss_crc64_update(&crc64,i);}ok&=check(x.value!=y.value&&p.value!=0U&&crc64!=0U,"baselines");
    ss256_init(&a); for (uint32_t i=0U;i<4U;++i) ss256_update(&a,i); ss256_update(&a,UINT32_MAX);
    ok&=check(a.lane[0]==UINT32_C(0xfb5d25d0)&&a.lane[1]==UINT32_C(0x3f120094)&&a.lane[2]==UINT32_C(0xa89056e6)&&a.lane[3]==UINT32_C(0x8ad91cbb)&&a.lane[4]==UINT32_C(0x392b8c5d)&&a.lane[5]==UINT32_C(0x8b325f98)&&a.lane[6]==UINT32_C(0xa96eb1cd)&&a.lane[7]==UINT32_C(0x157c8d3),"golden vector");
    { ss_affine512_t hleft,hright,hwhole,hcombined,hinv,hremoved; ss_affine512_init(&hleft);ss_affine512_init(&hright);ss_affine512_init(&hwhole);
      for(uint32_t i=0U;i<4U;++i) { ss_affine512_update(&hleft,i); }
      for(uint32_t i=4U;i<8U;++i) { ss_affine512_update(&hright,i); }
      for(uint32_t i=0U;i<8U;++i) { ss_affine512_update(&hwhole,i); }
      ss_affine512_combine(&hleft,&hright,&hcombined);ok&=check(ss_affine512_equal(&hwhole,&hcombined),"affine H combine");
      ss_affine512_inverse(&hright,&hinv);ss_affine512_combine(&hwhole,&hinv,&hremoved);ok&=check(ss_affine512_equal(&hleft,&hremoved),"affine H suffix inverse"); }
    printf("%s\n",ok?"PASS unit determinism properties":"FAIL"); return ok?0:1;
}
