#include <stdint.h>
#include <stdio.h>
#include "candidates.h"

static const uint32_t lengths[] = {16U,64U,256U,1024U,4096U,16384U,65536U,262144U,1048576U};
int main(void)
{
    printf("candidate,length,marker_hamming\n");
    for (int id=0; id<SS_CANDIDATE_COUNT; ++id) {
        for (uint32_t li=0U; li<(uint32_t)(sizeof(lengths)/sizeof(lengths[0])); ++li) {
            ss_state256_t marker, control; ss_candidate_init((ss_candidate_id)id,&marker); ss_candidate_init((ss_candidate_id)id,&control);
            ss_candidate_update((ss_candidate_id)id,&marker,UINT32_C(0xabcdef01));
            for (uint32_t i=1U;i<lengths[li];++i) { uint32_t x=i*UINT32_C(2654435761); ss_candidate_update((ss_candidate_id)id,&marker,x); ss_candidate_update((ss_candidate_id)id,&control,x); }
            printf("%s,%u,%u\n",ss_candidate_name((ss_candidate_id)id),lengths[li],ss256_hamming_distance(&marker,&control));
        }
    }
    return 0;
}
