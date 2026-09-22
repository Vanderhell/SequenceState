#include "sequence_state/trace.h"
#include "sequence_state/compose.h"
#include "sequence_state/range.h"

int main()
{
    ss_trace_state_t trace{};
    ss_compose_state_t compose{};
    ss_range_state_t range{};
    return ss_trace_init(&trace) | ss_compose_init(&compose) | ss_range_init(&range);
}
