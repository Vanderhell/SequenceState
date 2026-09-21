#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "candidates.h"

int main(void)
{
    ss_state256_t state; uint32_t passed=0U; const uint32_t cases=1000000U; clock_t begin;
    ss256_init(&state); begin=clock();
    for(uint32_t i=0U;i<cases;++i){ss_state256_t before=state;const uint32_t x=i*UINT32_C(2654435761);ss_candidate_update(SS_CANDIDATE_C,&state,x);ss_candidate_c_inverse(&state,x);if(ss256_equal(&before,&state)!=0)++passed;}
    printf("cases=%u passed=%u seconds=%.6f\n",cases,passed,(double)(clock()-begin)/(double)CLOCKS_PER_SEC); return passed==cases?0:1;
}
