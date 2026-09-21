#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "candidates.h"
int main(void){const uint32_t n=1000000U;for(int id=0;id<SS_CANDIDATE_COUNT;++id){ss_state256_t s;ss_candidate_init((ss_candidate_id)id,&s);clock_t start=clock();for(uint32_t i=0U;i<n;++i)ss_candidate_update((ss_candidate_id)id,&s,i*UINT32_C(2654435761));clock_t end=clock();double seconds=(double)(end-start)/(double)CLOCKS_PER_SEC;printf("%s state=32 bytes updates=%u seconds=%.6f updates_per_sec=%.0f hamming=%u\n",ss_candidate_name((ss_candidate_id)id),n,seconds,(double)n/seconds,ss256_hamming_distance(&s,&(ss_state256_t){{0U}}));}return 0;}
