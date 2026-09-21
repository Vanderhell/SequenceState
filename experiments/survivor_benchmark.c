#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "candidates.h"
#include "affine_h.h"
#include "h2.h"

int main(void)
{
    const uint32_t n=1000000U; clock_t start,end; ss_state256_t c,e; ss_affine512_t h; ss_h2_state_t h2;
    ss_candidate_init(SS_CANDIDATE_C,&c);start=clock();for(uint32_t i=0U;i<n;++i)ss_candidate_update(SS_CANDIDATE_C,&c,i*UINT32_C(2654435761));end=clock();printf("C_update,%u,%.6f\n",n,(double)(end-start)/(double)CLOCKS_PER_SEC);
    ss_candidate_init(SS_CANDIDATE_E,&e);start=clock();for(uint32_t i=0U;i<n;++i)ss_candidate_update(SS_CANDIDATE_E,&e,i*UINT32_C(2654435761));end=clock();printf("E_update,%u,%.6f\n",n,(double)(end-start)/(double)CLOCKS_PER_SEC);
    ss_affine512_init(&h);start=clock();for(uint32_t i=0U;i<n;++i)ss_affine512_update(&h,i*UINT32_C(2654435761));end=clock();printf("H_update,%u,%.6f\n",n,(double)(end-start)/(double)CLOCKS_PER_SEC);
    ss_h2_init(&h2);start=clock();for(uint32_t i=0U;i<n;++i)ss_h2_update(&h2,i*UINT32_C(2654435761));end=clock();printf("H2_update,%u,%.6f\n",n,(double)(end-start)/(double)CLOCKS_PER_SEC);
    ss_affine512_t h2a,h2b,hout;ss_affine512_init(&h2a);ss_affine512_init(&h2b);start=clock();for(uint32_t i=0U;i<n;++i)ss_affine512_combine(&h2a,&h2b,&hout);end=clock();printf("H_combine,%u,%.6f\n",n,(double)(end-start)/(double)CLOCKS_PER_SEC);
    return 0;
}
