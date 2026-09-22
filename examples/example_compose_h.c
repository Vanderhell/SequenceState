#include <stdio.h>
#include "sequence_state/compose.h"

int main(void)
{
    ss_compose_state_t left, right, whole, combined;
    if (ss_compose_init(&left) != 0 || ss_compose_init(&right) != 0 || ss_compose_init(&whole) != 0) return 1;
    for (unsigned int i = 0U; i < 4U; ++i) {
        ss_compose_update(&left, i); ss_compose_update(&whole, i);
    }
    for (unsigned int i = 4U; i < 8U; ++i) {
        ss_compose_update(&right, i); ss_compose_update(&whole, i);
    }
    if (ss_compose_combine(&left, &right, &combined) != 0) return 1;
    printf("composition=%s\n", ss_compose_equal(&whole, &combined) ? "exact" : "failed");
    return ss_compose_equal(&whole, &combined) ? 0 : 1;
}
