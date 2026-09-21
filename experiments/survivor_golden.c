#include <stdint.h>
#include <stdio.h>
#include "candidates.h"
#include "affine_h.h"
#include "h2.h"
#include "survivor_versions.h"

static void print_words(const ss_state256_t *s){for(uint32_t i=0U;i<8U;++i)printf("%08x",s->lane[i]);}
static void print_h(const ss_affine512_t *s){for(uint32_t i=0U;i<8U;++i)printf("%08x%08x",s->a[i],s->b[i]);}
static void print_h2(const ss_h2_state_t *s){printf("%016llx%016llx%016llx%016llx",(unsigned long long)s->length,(unsigned long long)s->polynomial,(unsigned long long)s->sum,(unsigned long long)s->moment);}
static void emit(const char *name,const uint32_t *x,uint32_t n)
{
    ss_state256_t c,e; ss_affine512_t h; ss_h2_state_t h2;ss_candidate_init(SS_CANDIDATE_C,&c);ss_candidate_init(SS_CANDIDATE_E,&e);ss_affine512_init(&h);ss_h2_init(&h2);
    for(uint32_t i=0U;i<n;++i){ss_candidate_update(SS_CANDIDATE_C,&c,x[i]);ss_candidate_update(SS_CANDIDATE_E,&e,x[i]);ss_affine512_update(&h,x[i]);ss_h2_update(&h2,x[i]);}
    printf("%s|%s|%s|",name,SS_C_VERSION,SS_E_VERSION);print_words(&c);printf("|");print_words(&e);printf("|%s|",SS_H_VERSION);print_h(&h);printf("|%s|",SS_H2_VERSION);print_h2(&h2);putchar('\n');
}
int main(void)
{
    uint32_t a[64],i; emit("empty",a,0U); a[0]=0U;emit("zero",a,1U);a[0]=UINT32_MAX;emit("maximum",a,1U);
    for(i=0U;i<64U;++i) { a[i]=i&1U; } emit("alternating",a,64U);
    for(i=0U;i<64U;++i) { a[i]=i; } emit("ascending",a,64U);
    for(i=0U;i<64U;++i) { a[i]=63U-i; } emit("descending",a,64U);
    {uint32_t trace[]={1U,2U,3U,4U,2U,3U,5U,10U,12U,11U,10U,12U,11U};emit("trace_protocol",trace,(uint32_t)(sizeof(trace)/sizeof(trace[0])));}
    for(i=0U;i<64U;++i){a[i]=i*UINT32_C(1664525)+UINT32_C(1013904223);}emit("random64",a,64U);
    for(i=0U;i<64U;++i) { a[i]=UINT32_C(0xa5a5a5a5); } emit("repeated",a,64U);
    return 0;
}
