#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_chip_info.h"
#include "esp_cpu.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include "affine_h.h"
#include "candidates.h"
#include "h2.h"
#include "sequence_state.h"
#include "survivor_versions.h"

static uint32_t rng_state = UINT32_C(0x13579bdf);
static uint32_t window_events[16384];
static uint32_t next_u32(void)
{
    rng_state = rng_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return rng_state;
}

static void print_state256(const ss_state256_t *s)
{
    for (int i = 0; i < 8; ++i) printf("%08" PRIx32, s->lane[i]);
}

static void print_affine(const ss_affine512_t *s)
{
    for (int i = 0; i < 8; ++i) printf("%08" PRIx32, s->a[i]);
    for (int i = 0; i < 8; ++i) printf("%08" PRIx32, s->b[i]);
}

static void print_h2(const ss_h2_state_t *s)
{
    printf("len=%" PRIu64 " poly=%016" PRIx64 " sum=%016" PRIx64 " moment=%016" PRIx64,
           s->length, s->polynomial, s->sum, s->moment);
}

static void boot_info(void)
{
    esp_chip_info_t info;
    esp_chip_info(&info);
    uint32_t flash_size = 0U;
    (void)esp_flash_get_size(esp_flash_default_chip, &flash_size);
    printf("SequenceState ESP32-S3 HW validation\n");
    printf("chip=%s revision=%d cores=%d cpu_mhz=%d flash=%" PRIu32 " psram=%" PRIu32 "\n",
           CONFIG_IDF_TARGET, info.revision, info.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
           flash_size, (uint32_t)esp_psram_get_size());
    printf("idf=%s compiler=%s versions C=%s E=%s H=%s H2=%s\n",
           esp_get_idf_version(), __VERSION__, SS_C_VERSION, SS_E_VERSION, SS_H_VERSION, SS_H2_VERSION);
}

static void golden_vectors(void)
{
    static const uint32_t symbols[] = {0U, UINT32_MAX, UINT32_C(0xaaaaaaaa), UINT32_C(0x55555555), 1U, 2U, 3U};
    ss_state256_t c, e;
    ss_affine512_t h;
    ss_h2_state_t h2;
    ss256_init(&c); ss_candidate_init(SS_CANDIDATE_E, &e); ss_affine512_init(&h); ss_h2_init(&h2);
    for (size_t i = 0U; i < sizeof(symbols) / sizeof(symbols[0]); ++i) {
        ss_candidate_update(SS_CANDIDATE_C, &c, symbols[i]);
        ss_candidate_update(SS_CANDIDATE_E, &e, symbols[i]);
        ss_affine512_update(&h, symbols[i]);
        ss_h2_update(&h2, symbols[i]);
    }
    printf("GOLDEN symbols=7 C="); print_state256(&c); printf(" E="); print_state256(&e);
    printf(" H="); print_affine(&h); printf(" H2="); print_h2(&h2); printf("\n");
}

static int c_roundtrips(void)
{
    int failures = 0;
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_state256_t before, after;
        ss256_init(&before);
        for (uint32_t j = 0U; j < (i & 31U); ++j) ss_candidate_update(SS_CANDIDATE_C, &before, next_u32());
        after = before;
        const uint32_t symbol = next_u32();
        ss_candidate_update(SS_CANDIDATE_C, &after, symbol);
        ss_candidate_c_inverse(&after, symbol);
        if (!ss256_equal(&before, &after)) { ++failures; break; }
    }
    return failures;
}

static int c_multi_rollback(void)
{
    int failures = 0;
    for (uint32_t i = 0U; i < 10000U; ++i) {
        ss_state256_t state;
        uint32_t symbols[32];
        uint32_t n = (i & 31U) + 1U;
        ss256_init(&state);
        for (uint32_t j = 0U; j < n; ++j) { symbols[j] = next_u32(); ss_candidate_update(SS_CANDIDATE_C, &state, symbols[j]); }
        for (uint32_t j = n; j > 0U; --j) ss_candidate_c_inverse(&state, symbols[j - 1U]);
        ss_state256_t initial;
        ss256_init(&initial);
        if (!ss256_equal(&state, &initial)) { failures = 1; break; }
    }
    return failures;
}

static int e_roundtrips(void)
{
    /* E has no inverse API: this intentionally records whether the state is
       accidentally reversible, rather than hiding the missing guarantee. */
    int failures = 0;
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_state256_t a, b;
        ss_candidate_init(SS_CANDIDATE_E, &a);
        b = a;
        ss_candidate_update(SS_CANDIDATE_E, &a, next_u32());
        if (ss256_equal(&a, &b)) ++failures;
    }
    return failures;
}

static int affine_tests(void)
{
    int failures = 0;
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_affine512_t a, b, c, ab, ab_c, a_bc, bc;
        ss_affine512_init(&a); ss_affine512_init(&b); ss_affine512_init(&c);
        const uint32_t na = i & 7U, nb = (i * 3U) & 7U, nc = (i * 5U) & 7U;
        for (uint32_t j = 0U; j < na; ++j) ss_affine512_update(&a, next_u32());
        for (uint32_t j = 0U; j < nb; ++j) ss_affine512_update(&b, next_u32());
        for (uint32_t j = 0U; j < nc; ++j) ss_affine512_update(&c, next_u32());
        ss_affine512_combine(&a, &b, &ab); ss_affine512_combine(&ab, &c, &ab_c);
        ss_affine512_combine(&b, &c, &bc); ss_affine512_combine(&a, &bc, &a_bc);
        if (!ss_affine512_equal(&ab_c, &a_bc)) { ++failures; break; }
        if ((i & 255U) == 255U) vTaskDelay(1);
    }
    return failures;
}

static int h2_tests(void)
{
    int failures = 0;
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_h2_state_t all, left, right, combined, suffix;
        ss_h2_init(&all); ss_h2_init(&left); ss_h2_init(&right);
        const uint32_t n = i & 63U, split = n / 2U;
        for (uint32_t j = 0U; j < n; ++j) {
            const uint32_t x = next_u32(); ss_h2_update(&all, x);
            if (j < split) ss_h2_update(&left, x); else ss_h2_update(&right, x);
        }
        ss_h2_combine(&left, &right, &combined);
        if (!ss_h2_equal(&all, &combined) || !ss_h2_remove_prefix(&all, &left, &suffix)) { ++failures; break; }
        if (!ss_h2_equal(&suffix, &right)) { ++failures; break; }
    }
    return failures;
}

static int h_chunk_tests(void)
{
    int failures = 0;
    for (uint32_t i = 0U; i < 100000U; ++i) {
        ss_affine512_t whole, left, right, combined;
        ss_affine512_init(&whole); ss_affine512_init(&left); ss_affine512_init(&right);
        const uint32_t n = (i & 63U) + 1U, split = (i * 17U) % n;
        for (uint32_t j = 0U; j < n; ++j) {
            const uint32_t x = next_u32();
            ss_affine512_update(&whole, x);
            if (j < split) ss_affine512_update(&left, x); else ss_affine512_update(&right, x);
        }
        ss_affine512_combine(&left, &right, &combined);
        if (!ss_affine512_equal(&whole, &combined)) { failures = 1; break; }
        if ((i & 255U) == 255U) vTaskDelay(1);
    }
    return failures;
}

static int h2_window_tests(void)
{
    static const uint32_t sizes[] = {16U, 64U, 256U, 1024U, 4096U, 16384U};
    int failures = 0;
    for (size_t k = 0U; k < sizeof(sizes) / sizeof(sizes[0]); ++k) {
        const uint32_t n = sizes[k];
        ss_h2_state_t rolling;
        ss_h2_init(&rolling);
        for (uint32_t i = 0U; i < n; ++i) { window_events[i] = next_u32(); ss_h2_update(&rolling, window_events[i]); }
        const uint32_t steps = n > 4096U ? 32U : 256U;
        for (uint32_t step = 0U; step < steps; ++step) {
            const uint32_t incoming = next_u32();
            ss_h2_state_t incoming_state, appended, next, reference, drop;
            ss_h2_init(&incoming_state); ss_h2_update(&incoming_state, incoming);
            ss_h2_combine(&rolling, &incoming_state, &appended);
            ss_h2_init(&drop); ss_h2_update(&drop, window_events[step % n]);
            if (!ss_h2_remove_prefix(&appended, &drop, &next)) { failures = 1; break; }
            window_events[step % n] = incoming;
            ss_h2_init(&reference);
            for (uint32_t j = 0U; j < n; ++j) {
                ss_h2_update(&reference, window_events[(step + 1U + j) % n]);
                if ((j & 255U) == 255U) vTaskDelay(1);
            }
            if (!ss_h2_equal(&next, &reference)) { failures = 1; break; }
            rolling = next;
        }
        if (failures != 0) break;
    }
    return failures;
}

static int trace_mutation_tests(void)
{
    int c_detected = 0, e_detected = 0, h2_detected = 0;
    for (uint32_t trial = 0U; trial < 10000U; ++trial) {
        ss_state256_t c_ref, c_mut, e_ref, e_mut;
        ss_h2_state_t h2_ref, h2_mut;
        ss256_init(&c_ref); ss256_init(&c_mut);
        ss_candidate_init(SS_CANDIDATE_E, &e_ref); ss_candidate_init(SS_CANDIDATE_E, &e_mut);
        ss_h2_init(&h2_ref); ss_h2_init(&h2_mut);
        const uint32_t changed = trial % 32U;
        for (uint32_t i = 0U; i < 32U; ++i) {
            const uint32_t symbol = (i * UINT32_C(0x9e3779b9)) ^ (trial * 17U);
            const uint32_t mutated = i == changed ? symbol ^ UINT32_C(0x01010101) : symbol;
            ss_candidate_update(SS_CANDIDATE_C, &c_ref, symbol);
            ss_candidate_update(SS_CANDIDATE_C, &c_mut, mutated);
            ss_candidate_update(SS_CANDIDATE_E, &e_ref, symbol);
            ss_candidate_update(SS_CANDIDATE_E, &e_mut, mutated);
            ss_h2_update(&h2_ref, symbol); ss_h2_update(&h2_mut, mutated);
        }
        c_detected += !ss256_equal(&c_ref, &c_mut);
        e_detected += !ss256_equal(&e_ref, &e_mut);
        h2_detected += !ss_h2_equal(&h2_ref, &h2_mut);
        if ((trial & 255U) == 255U) vTaskDelay(1);
    }
    printf("TRACE mutation_trials=10000 C_detected=%d E_detected=%d H2_detected=%d\n", c_detected, e_detected, h2_detected);
    return (c_detected == 10000 && e_detected == 10000 && h2_detected == 10000) ? 0 : 1;
}

static uint64_t timed_us(void (*fn)(void), uint32_t repetitions)
{
    const int64_t start = esp_timer_get_time();
    for (uint32_t i = 0U; i < repetitions; ++i) fn();
    return (uint64_t)(esp_timer_get_time() - start);
}

static ss_state256_t bench_c_state;
static ss_affine512_t bench_h_state, bench_h_other, bench_h_out;
static ss_h2_state_t bench_h2_state, bench_h2_other, bench_h2_out;
static uint32_t bench_watchdog_counter;
static void bench_watchdog(void)
{
    if ((bench_watchdog_counter++ & UINT32_C(0xffff)) == 0U) {
        (void)esp_task_wdt_reset();
        vTaskDelay(1);
    }
}
static void bench_c(void) { bench_watchdog(); ss_candidate_update(SS_CANDIDATE_C, &bench_c_state, next_u32()); }
static void bench_e(void) { bench_watchdog(); ss_candidate_update(SS_CANDIDATE_E, &bench_c_state, next_u32()); }
static void bench_c_inverse(void) { bench_watchdog(); const uint32_t x = next_u32(); ss_candidate_update(SS_CANDIDATE_C, &bench_c_state, x); ss_candidate_c_inverse(&bench_c_state, x); }
static void bench_h(void) { bench_watchdog(); ss_affine512_update(&bench_h_state, next_u32()); }
static void bench_h_combine(void) { bench_watchdog(); ss_affine512_combine(&bench_h_state, &bench_h_other, &bench_h_out); }
static void bench_h2(void) { bench_watchdog(); ss_h2_update(&bench_h2_state, next_u32()); }
static void bench_h2_combine(void) { bench_watchdog(); ss_h2_combine(&bench_h2_state, &bench_h2_other, &bench_h2_out); }

static void benchmark_one(const char *name, void (*fn)(void), uint32_t count)
{
    (void)timed_us(fn, 1000U);
    const esp_cpu_cycle_count_t cycle_start = esp_cpu_get_cycle_count();
    const uint64_t us = timed_us(fn, count);
    const esp_cpu_cycle_count_t cycles = esp_cpu_get_cycle_count() - cycle_start;
    printf("BENCH op=%s count=%" PRIu32 " us=%" PRIu64 " cycles=%" PRIu32 " cycles_per_op=%" PRIu32 " ns_per_op=%" PRIu64 " ops_per_s=%" PRIu64 "\n",
           name, count, us, (uint32_t)cycles, (uint32_t)(cycles / count), (us * 1000U) / count, (us == 0U) ? 0U : ((uint64_t)count * 1000000U) / us);
}

static void benchmarks(void)
{
    ss256_init(&bench_c_state); ss_affine512_init(&bench_h_state); ss_affine512_init(&bench_h_other);
    ss_h2_init(&bench_h2_state); ss_h2_init(&bench_h2_other);
    benchmark_one("C.update", bench_c, 1000000U);
    benchmark_one("C.forward_inverse", bench_c_inverse, 500000U);
    benchmark_one("E.update", bench_e, 1000000U);
    benchmark_one("H.update", bench_h, 1000000U);
    benchmark_one("H.combine", bench_h_combine, 1000000U);
    benchmark_one("H2.update", bench_h2, 1000000U);
    benchmark_one("H2.combine", bench_h2_combine, 1000000U);
}

static int long_run(void)
{
    /* Ten million events was attempted, but the IDF task watchdog fired in
       the combined C/H/H2 stress loop after the first checkpoint. Keep the
       repeatable hardware validation run at the completed 1M point and report
       the 10M watchdog limitation explicitly. */
    const uint32_t targets[] = {1000000U};
    for (size_t k = 0U; k < sizeof(targets) / sizeof(targets[0]); ++k) {
        ss_state256_t c; ss_affine512_t h; ss_h2_state_t h2;
        ss256_init(&c); ss_affine512_init(&h); ss_h2_init(&h2);
        const int64_t start = esp_timer_get_time();
        for (uint32_t i = 0U; i < targets[k]; ++i) {
            const uint32_t x = i * UINT32_C(2654435761) ^ UINT32_C(0x9e3779b9);
            ss_candidate_update(SS_CANDIDATE_C, &c, x); ss_affine512_update(&h, x); ss_h2_update(&h2, x);
            if ((i & UINT32_C(0xffff)) == UINT32_C(0xffff)) {
                (void)esp_task_wdt_reset();
                vTaskDelay(1);
            }
        }
        printf("LONG events=%" PRIu32 " us=%" PRId64 " C=", targets[k], esp_timer_get_time() - start);
        print_state256(&c); printf(" H2="); print_h2(&h2); printf(" heap=%u\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
    return 0;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);
    boot_info();
    printf("HEAP initial_internal=%u psram=%u\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    golden_vectors();
    const int c_failures = c_roundtrips();
    const int c_rollback_failures = c_multi_rollback();
    const int e_nonidentity = e_roundtrips();
    const int h_failures = affine_tests();
    const int h_chunk_failures = h_chunk_tests();
    const int h2_failures = h2_tests();
    const int h2_window_failures = h2_window_tests();
    const int trace_failures = trace_mutation_tests();
    printf("TEST C_roundtrips=%s failures=%d\n", c_failures == 0 ? "PASS" : "FAIL", c_failures);
    printf("TEST C_multi_rollback=%s failures=%d\n", c_rollback_failures == 0 ? "PASS" : "FAIL", c_rollback_failures);
    printf("TEST E_reversibility_probe=%s nonidentity=%d (E has no inverse guarantee)\n", e_nonidentity == 0 ? "PASS" : "FAIL", e_nonidentity);
    printf("TEST H_associativity=%s failures=%d\n", h_failures == 0 ? "PASS" : "FAIL", h_failures);
    printf("TEST H_chunk_composition=%s failures=%d\n", h_chunk_failures == 0 ? "PASS" : "FAIL", h_chunk_failures);
    printf("TEST H2_algebra=%s failures=%d\n", h2_failures == 0 ? "PASS" : "FAIL", h2_failures);
    printf("TEST H2_sliding_windows=%s failures=%d\n", h2_window_failures == 0 ? "PASS" : "FAIL", h2_window_failures);
    printf("TEST trace_mutation=%s failures=%d\n", trace_failures == 0 ? "PASS" : "FAIL", trace_failures);
    benchmarks();
    (void)long_run();
    printf("HEAP final_internal=%u psram=%u stack_high_water_bytes=%u\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)(uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t)));
    printf("DONE\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
