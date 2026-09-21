# SEQUENCESTATE ESP32-S3 HARDWARE VALIDATION

## HARDWARE

- Chip: ESP32-S3, revision 0.2, two cores, 240 MHz.
- Flash: 16,777,216 bytes.
- PSRAM: 8,388,608 bytes, octal, detected and memory-tested by ESP-IDF.
- Port: COM37, USB-Serial/JTAG, MAC `3c:0f:02:d9:77:30`.
- ESP-IDF: v5.5.1.
- Compiler: xtensa-esp-elf GCC 14.2.0.
- Builds: IDF performance (`-O3`) and IDF size/release (`-Os`). A debug `-Og` image was also run before the release matrix.

## BUILD AND INTEGRATION

The target is isolated under `platform/esp32`. It links the existing C11 core and survivor sources directly; no ESP32-specific algorithm fork was added. SequenceState state and test buffers are internal-RAM objects. PSRAM is enabled for board validation but is not used by the primitives.

Application image sizes:

| configuration | app image |
|---|---:|
| debug `-Og` | 208,384 B |
| `-Os` | 190,048 B |
| `-O3` | 201,248 B |

All builds fit the 1 MiB application partition.

## GOLDEN VECTORS AND CORRECTNESS

The ESP32 output for C, E, H and H2 matched the frozen desktop vectors byte-for-byte. The on-device suite passed:

- C: 100,000 forward/known-symbol inverse round trips and 10,000 multi-event rollbacks.
- H: 100,000 associativity checks and 100,000 independently-built chunk compositions.
- H2: 100,000 algebra checks and exact sliding windows of 16, 64, 256, 1024, 4096 and 16384 events.
- Trace mutation smoke workload: 10,000 single-event mutations detected by C, E and H2.
- 1,000,000-event combined stream: completed with `DONE`.

The E test is intentionally not called an inverse test: E has no inverse operation or mathematical rollback guarantee. Its mutation signal was tested only for state inequality.

## PERFORMANCE — O3, 240 MHz

Measurements use one million operations after warmup. Serial output is outside timed loops. A watchdog-safe periodic yield is included; therefore these are application-level sustained measurements, not interrupt-disabled idealized inner-loop numbers.

| primitive / operation | state | cycles/op | ns/op |
|---|---:|---:|---:|
| C update | 32 B | 237 | 991 |
| C forward + inverse | 32 B | 505 | 2,104 |
| E update | 32 B | 249 | 1,038 |
| H update | 64 B | 289 | 1,205 |
| H combine | 64 B | 146 | 609 |
| H2 update | 32 B | 106 | 442 |
| H2 combine | 32 B | 98 | 411 |

The corresponding `-Os` results were C 252, E 285, H 304, H combine 153, H2 132 and H2 combine 121 cycles/op. Thus H2 is the fastest survivor in both tested optimization profiles; H is expensive to update but materially cheaper to combine than replaying a block.

## MEMORY, STACK AND PSRAM

- C/E state: 32 bytes.
- H state: 64 bytes.
- H2 state: 32 bytes.
- O3 task stack high-water mark: 1,608 bytes; `-Os`: 1,624 bytes.
- Internal free heap before/after O3 run: 326,288 / 326,288 bytes.
- PSRAM free before/after O3 run: 8,386,192 / 8,386,192 bytes.
- Dynamic allocation by the portable primitives: none observed.
- PSRAM required by SequenceState: no.

The stack value is for the ESP-IDF benchmark task, not a claim that every caller needs 1,608 bytes; primitive state itself is fixed-size and has no heap dependency.

## C — STRONG TRACE

Mathematical behavior is the existing reversible 256-bit per-symbol transform. For every tested symbol, `inverse(forward(S,x),x)==S` held bit-for-bit. Hardware results: 237 cycles/update, 505 cycles for forward plus inverse, 32-byte state, 10,000/10,000 multi-event rollback, and 10,000/10,000 single-mutation detection trials.

Best justified use: known-event rollback and reversible execution/diagnostic trace state where the removed event is available. C is not a semantic trace decoder and the mutation smoke test does not establish anomaly-classification accuracy.

Verdict: **ACCEPTED as a specialized reversible trace primitive**.

## E — LIGHT TRACE

E is a 32-byte in-place permutation state. It is order-sensitive, but the current implementation does not expose a valid inverse and the earlier snapshot-inverse interpretation was incorrect. On hardware E was slower than C in both release profiles: 249 vs 237 cycles at O3 and 285 vs 252 at Os. It therefore has neither a rollback guarantee nor a measured cost advantage.

Verdict: **REDUNDANT**. Do not expose it as a production survivor; retain only as historical research code and regression context.

## H — EXACT COMPOSE

H stores eight affine transforms `T_i(s)=a_i*s+b_i (mod 2^32)`. A symbol update composes the per-symbol odd multiplier and offset into each lane. Block composition is exact and associative because affine-function composition is associative; all arithmetic uses defined unsigned wraparound.

Hardware confirmed 100,000 associativity identities and 100,000 independently-built chunk compositions. The 64-byte state costs 289 cycles for replaying one event but only 146 cycles to combine two already-built block states. No raw event replay is required when producers transmit H summaries and the global block order is known.

Best justified use: DMA/network/storage block summaries, deterministic tree reduction and distributed ordered-chunk aggregation. It is a sequence algebra, not a cryptographic integrity primitive and not a recoverable history.

Verdict: **ACCEPTED**.

## H2 — EXACT RANGE / WINDOW

H2 stores four 64-bit lanes: length, polynomial state, symbol sum and position moment. Each lane has an exact concatenation rule; prefix removal is exact under modular arithmetic. The hardware suite confirmed associativity/concatenation, prefix removal through the algebra test, and exact windows through 16384 events with explicit outgoing-event storage in the harness.

At O3 H2 costs 106 cycles/update and 98 cycles/combine; at Os it costs 132 and 121. The 32-byte state is cheaper and faster than H, while carrying more low-order position/content information. Window maintenance is not free: an application must retain outgoing symbols or suitable block summaries. H2 range localization and hierarchical-log complexity remain supported by the desktop experiments; this board harness directly validated the algebra and window primitive, not a full 65,536-node localization tree.

Best justified use: exact composable rolling windows and structured log/trace summaries where length, polynomial identity, sum and position moment are useful together.

Verdict: **ACCEPTED**.

## C VS E

C and E do not occupy distinct Pareto positions on this hardware. C is faster at O3 and Os, has the only tested exact inverse, and has the same 32-byte state. E is therefore not a lightweight reversible primitive in the current implementation. The correct firmware choice is C when reversible trace state is required; no evidence supports choosing E.

## H VS H2

H and H2 are distinct. H provides a larger pure affine transformation summary and its primary value is exact transformation/block composition. H2 is smaller, faster and query-oriented through polynomial, sum and moment lanes, with exact prefix removal and window algebra. H wins as the minimal affine transformation algebra; H2 wins as the compact range/window summary. Neither should be represented as a universal sequence memory.

## ROBUSTNESS

- Deterministic boot/golden output: PASS across desktop GCC/MSVC/Clang and ESP32-S3.
- Boundary and structured vectors: included in the frozen corpus and hardware golden run.
- C rollback: PASS for 100,000 single-event and 10,000 multi-event cases on device.
- H/H2 algebra: PASS for 100,000-class on-device tests and exact window sizes 16–16384.
- Long run: 1,000,000 combined events PASS, no reset, no final state anomaly, no heap drift.
- 10,000-event mutation workload: PASS for C, E and H2.
- A 10M combined run was attempted before watchdog-safe pacing and produced task-WDT diagnostics after the first checkpoint; it is not claimed as complete. The final 1M run uses periodic safe yields and completes cleanly.
- Sanitizers were not available in the Windows MinGW environment; this hardware target used compiler warnings, IDF build checks, golden vectors and runtime boundary tests instead.

## FINAL CLASSIFICATION

- C: **ACCEPTED — specialized reversible trace primitive**.
- E: **REDUNDANT**.
- H: **ACCEPTED — exact composable affine sequence algebra**.
- H2: **ACCEPTED — exact composable range/window state**.

## FINAL HARDWARE VERDICT

Three primitives are justified by this ESP32-S3 evidence: C for exact known-event rollback, H for exact ordered block composition, and H2 for smaller/faster exact composition with range/window-oriented lanes. E is not justified as a fourth API.

## MOST IMPORTANT HARDWARE RESULT

At 240 MHz `-O3`, H2 combines two 32-byte exact sequence summaries in **98 cycles** and updates in **106 cycles**, while passing exact algebra and 16–16384-event window tests on the real ESP32-S3.

## NEXT STEP

Expose and document only the three justified APIs—reversible trace C, composable affine H and range/window H2—then add the ESP32 golden/cost harness to continuous cross-target validation; do not promote E.
