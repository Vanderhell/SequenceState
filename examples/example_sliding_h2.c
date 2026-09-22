#include <stdio.h>
#include "sequence_state/range.h"

int main(void)
{
    ss_range_state_t window, incoming, appended, outgoing, next;
    if (ss_range_init(&window) != 0 || ss_range_init(&incoming) != 0 || ss_range_init(&outgoing) != 0) return 1;
    for (unsigned int i = 0U; i < 4U; ++i) ss_range_update(&window, i);
    ss_range_update(&incoming, 4U); ss_range_update(&outgoing, 0U);
    if (ss_range_combine(&window, &incoming, &appended) != 0) return 1;
    if (ss_range_remove_prefix(&appended, &outgoing, &next) != 1) return 1;
    printf("window_length=%llu\n", (unsigned long long)next.length);
    return next.length == 4U ? 0 : 1;
}
