#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_cpu.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sequence_state/compose.h"
#include "sequence_state/range.h"
#include "sequence_state/trace.h"
#include "sequence_state/version.h"

static uint32_t rng_state = UINT32_C(0x13579bdf);
static uint32_t window_events[16384];
static uint32_t next_u32(void)
{
    rng_state = rng_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return rng_state;
}

static void service_watchdog(void)
{
    (void)esp_task_wdt_reset();
    vTaskDelay(1);
}

static void print_trace(const ss_trace_state_t *state)
{
    for (uint32_t i = 0U; i < 8U; ++i) printf("%08" PRIx32, state->lane[i]);
}

static void print_range(const ss_range_state_t *state)
{
    printf("len=%" PRIu64 " poly=%016" PRIx64 " sum=%016" PRIx64 " moment=%016" PRIx64,
           state->length, state->polynomial, state->sum, state->moment);
}

static void boot_info(void)
{
    esp_chip_info_t info; uint32_t flash_size = 0U;
    esp_chip_info(&info); (void)esp_flash_get_size(esp_flash_default_chip, &flash_size);
    printf("SequenceState ESP32-S3 release validation\n");
    printf("chip=%s revision=%d cores=%d cpu_mhz=%d flash=%" PRIu32 " psram=%" PRIu32 "\n",
           CONFIG_IDF_TARGET, info.revision, info.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
           flash_size, (uint32_t)esp_psram_get_size());
    printf("idf=%s compiler=%s versions C=%s H=%s H2=%s\n", esp_get_idf_version(), __VERSION__,
           SEQUENCE_STATE_TRACE_VERSION, SEQUENCE_STATE_COMPOSE_VERSION, SEQUENCE_STATE_RANGE_VERSION);
}

static int trace_tests(void)
{
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_trace_state_t state, before;
        ss_trace_init(&state);
        for (uint32_t j = 0U; j < (i & 31U); ++j) ss_trace_update(&state, next_u32());
        before = state; const uint32_t symbol = next_u32();
        ss_trace_update(&state, symbol); ss_trace_inverse_update(&state, symbol);
        if (!ss_trace_equal(&state, &before)) return 1;
        if ((i & 8191U) == 8191U) service_watchdog();
    }
    for (uint32_t i = 0U; i < 10000U; ++i) {
        ss_trace_state_t state, initial; uint32_t symbols[32]; const uint32_t n = (i & 31U) + 1U;
        ss_trace_init(&state); initial = state;
        for (uint32_t j = 0U; j < n; ++j) { symbols[j] = next_u32(); ss_trace_update(&state, symbols[j]); }
        for (uint32_t j = n; j > 0U; --j) ss_trace_inverse_update(&state, symbols[j - 1U]);
        if (!ss_trace_equal(&state, &initial)) return 2;
    }
    return 0;
}

static int compose_tests(void)
{
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_compose_state_t all, left, right, combined, inverse, recovered;
        ss_compose_init(&all); ss_compose_init(&left); ss_compose_init(&right);
        const uint32_t n = i & 63U, split = (i * 17U) % (n + 1U);
        for (uint32_t j = 0U; j < n; ++j) {
            const uint32_t x = next_u32(); ss_compose_update(&all, x);
            if (j < split) ss_compose_update(&left, x); else ss_compose_update(&right, x);
        }
        ss_compose_combine(&left, &right, &combined);
        ss_compose_inverse(&right, &inverse); ss_compose_combine(&all, &inverse, &recovered);
        if (!ss_compose_equal(&all, &combined) || !ss_compose_equal(&left, &recovered)) return 1;
        if ((i & 8191U) == 8191U) service_watchdog();
    }
    return 0;
}

static int range_tests(void)
{
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_range_state_t all, left, right, combined, suffix;
        ss_range_init(&all); ss_range_init(&left); ss_range_init(&right);
        const uint32_t n = i & 63U, split = n / 2U;
        for (uint32_t j = 0U; j < n; ++j) {
            const uint32_t x = next_u32(); ss_range_update(&all, x);
            if (j < split) ss_range_update(&left, x); else ss_range_update(&right, x);
        }
        ss_range_combine(&left, &right, &combined);
        if (ss_range_remove_prefix(&all, &left, &suffix) != 1 || !ss_range_equal(&all, &combined)
            || !ss_range_equal(&suffix, &right)) return 1;
        if ((i & 8191U) == 8191U) service_watchdog();
    }
    return 0;
}

static int window_tests(void)
{
    static const uint32_t sizes[] = {16U, 64U, 256U, 1024U, 4096U, 16384U};
    for (uint32_t k = 0U; k < sizeof(sizes) / sizeof(sizes[0]); ++k) {
        const uint32_t n = sizes[k]; ss_range_state_t rolling; ss_range_init(&rolling);
        for (uint32_t i = 0U; i < n; ++i) { window_events[i] = next_u32(); ss_range_update(&rolling, window_events[i]); }
        const uint32_t steps = n > 4096U ? 16U : 64U;
        for (uint32_t step = 0U; step < steps; ++step) {
            ss_range_state_t incoming, appended, outgoing, next, reference;
            const uint32_t value = next_u32(); ss_range_init(&incoming); ss_range_update(&incoming, value);
            ss_range_combine(&rolling, &incoming, &appended); ss_range_init(&outgoing);
            ss_range_update(&outgoing, window_events[step % n]);
            if (ss_range_remove_prefix(&appended, &outgoing, &next) != 1) return 1;
            window_events[step % n] = value; ss_range_init(&reference);
            for (uint32_t j = 0U; j < n; ++j) ss_range_update(&reference, window_events[(step + 1U + j) % n]);
            if (!ss_range_equal(&next, &reference)) return 2;
            rolling = next;
        }
        service_watchdog();
    }
    return 0;
}

static int mutation_tests(void)
{
    for (uint32_t trial = 0U; trial < 10000U; ++trial) {
        ss_trace_state_t a, b; ss_range_state_t x, y;
        ss_trace_init(&a); ss_trace_init(&b); ss_range_init(&x); ss_range_init(&y);
        const uint32_t changed = trial & 31U;
        for (uint32_t i = 0U; i < 32U; ++i) {
            const uint32_t value = i * UINT32_C(0x9e3779b9) ^ trial * 17U;
            const uint32_t changed_value = i == changed ? value ^ UINT32_C(0x01010101) : value;
            ss_trace_update(&a, value); ss_trace_update(&b, changed_value);
            ss_range_update(&x, value); ss_range_update(&y, changed_value);
        }
        if (ss_trace_equal(&a, &b) || ss_range_equal(&x, &y)) return 1;
        if ((trial & 8191U) == 8191U) service_watchdog();
    }
    return 0;
}

static ss_trace_state_t bench_trace;
static ss_compose_state_t bench_compose, bench_compose_other, bench_compose_out;
static ss_range_state_t bench_range, bench_range_other, bench_range_out;
static uint32_t bench_counter;
static void bench_service(void) { if ((bench_counter++ & UINT32_C(0xffff)) == 0U) service_watchdog(); }
static void bench_trace_update(void) { bench_service(); ss_trace_update(&bench_trace, next_u32()); }
static void bench_trace_inverse(void) { bench_service(); const uint32_t x = next_u32(); ss_trace_update(&bench_trace, x); ss_trace_inverse_update(&bench_trace, x); }
static void bench_compose_update(void) { bench_service(); ss_compose_update(&bench_compose, next_u32()); }
static void bench_compose_combine(void) { bench_service(); ss_compose_combine(&bench_compose, &bench_compose_other, &bench_compose_out); }
static void bench_range_update(void) { bench_service(); ss_range_update(&bench_range, next_u32()); }
static void bench_range_combine(void) { bench_service(); ss_range_combine(&bench_range, &bench_range_other, &bench_range_out); }

static void benchmark_one(const char *name, void (*function)(void), uint32_t count)
{
    for (uint32_t i = 0U; i < 1000U; ++i) function();
    const esp_cpu_cycle_count_t start = esp_cpu_get_cycle_count();
    const int64_t time_start = esp_timer_get_time();
    for (uint32_t i = 0U; i < count; ++i) function();
    const uint32_t cycles = (uint32_t)(esp_cpu_get_cycle_count() - start);
    const uint64_t us = (uint64_t)(esp_timer_get_time() - time_start);
    printf("BENCH op=%s count=%" PRIu32 " us=%" PRIu64 " cycles=%" PRIu32 " cycles_per_op=%" PRIu32 " ns_per_op=%" PRIu64 " ops_per_s=%" PRIu64 "\n",
           name, count, us, cycles, cycles / count, (us * 1000U) / count,
           us == 0U ? 0U : ((uint64_t)count * 1000000U) / us);
}

static void benchmarks(void)
{
    ss_trace_init(&bench_trace); ss_compose_init(&bench_compose); ss_compose_init(&bench_compose_other);
    ss_range_init(&bench_range); ss_range_init(&bench_range_other); bench_counter = 0U;
    benchmark_one("C.update", bench_trace_update, 1000000U);
    benchmark_one("C.forward_inverse", bench_trace_inverse, 500000U);
    benchmark_one("H.update", bench_compose_update, 1000000U);
    benchmark_one("H.combine", bench_compose_combine, 1000000U);
    benchmark_one("H2.update", bench_range_update, 1000000U);
    benchmark_one("H2.combine", bench_range_combine, 1000000U);
}

static void long_run(void)
{
    ss_trace_state_t trace; ss_compose_state_t compose; ss_range_state_t range;
    ss_trace_init(&trace); ss_compose_init(&compose); ss_range_init(&range);
    const int64_t start = esp_timer_get_time();
    for (uint32_t i = 0U; i < 1000000U; ++i) {
        const uint32_t x = i * UINT32_C(2654435761) ^ UINT32_C(0x9e3779b9);
        ss_trace_update(&trace, x); ss_compose_update(&compose, x); ss_range_update(&range, x);
        if ((i & UINT32_C(0xffff)) == UINT32_C(0xffff)) service_watchdog();
    }
    printf("LONG events=1000000 us=%" PRId64 " C=", esp_timer_get_time() - start); print_trace(&trace);
    printf(" H2="); print_range(&range); printf(" heap=%u\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

void app_main(void)
{
    boot_info();
    (void)esp_task_wdt_add(NULL);
    const size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t psram_before = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    printf("HEAP initial_internal=%u psram=%u\n", (unsigned)heap_before, (unsigned)psram_before);
    const int c = trace_tests(), h = compose_tests(), h2 = range_tests(), window = window_tests(), mutation = mutation_tests();
    printf("TEST C_release=%s failures=%d\n", c == 0 ? "PASS" : "FAIL", c);
    printf("TEST H_release=%s failures=%d\n", h == 0 ? "PASS" : "FAIL", h);
    printf("TEST H2_release=%s failures=%d\n", h2 == 0 ? "PASS" : "FAIL", h2);
    printf("TEST H2_windows=%s failures=%d\n", window == 0 ? "PASS" : "FAIL", window);
    printf("TEST trace_mutation=%s failures=%d\n", mutation == 0 ? "PASS" : "FAIL", mutation);
    benchmarks(); long_run();
    printf("HEAP final_internal=%u psram=%u\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("STACK task_high_water_bytes=%u\n", (unsigned)(uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t)));
    printf("DONE\n");
}
