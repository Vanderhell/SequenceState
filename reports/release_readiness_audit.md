# SequenceState v0.1.0 release-readiness audit

## Scope

The production target now contains only C-1.0, H-1.0 and H2-1.0. Historical
candidates and baselines are behind `SS_BUILD_RESEARCH=ON` and are not in the
installed library or public include path.

## Production API

* `sequence_state/trace.h`: 32-byte caller-owned C state, checked update and
  known-symbol inverse.
* `sequence_state/compose.h`: 64-byte caller-owned H affine state, checked
  update/combine/inverse. Combine is exact, associative and output-alias-safe.
* `sequence_state/range.h`: 32-byte caller-owned H2 state, checked
  update/combine/prefix removal. Operations are exact modulo their documented
  unsigned domains and output-alias-safe.

Invalid pointer arguments have deterministic status results. No production
function allocates memory, recurses or uses mutable global algorithm state.

## Host and cross-target evidence

The following all passed from clean out-of-tree builds:

* GCC 16.1 Release, Debug/strict, `-O2` and `-Os`.
* MSVC 19.42 Release.
* Clang-cl 19.1.5 Release.
* C++11 public-header smoke test.
* 1,000,000 C rollback properties; 100,000 H algebra/inverse cases; 100,000
  H2 composition/range cases; exhaustive-style boundary/null/alias tests.
* cppcheck 2.20 with warning/style/performance/portability checks.
* Freestanding warning-clean compile for Cortex-M0 and Cortex-M4 with Clang
  and RV32IM with Espressif GCC 14.2.0.

ASan/UBSan were attempted with MinGW GCC and Clang-cl. MinGW lacks
`libasan`/`libubsan`; Clang-cl cannot link the installed debug runtime with
`/fsanitize=address`. WSL is installed but distro enumeration is denied in the
environment. No sanitizer PASS is claimed. `SS_BUILD_FUZZ` provides a real
Clang libFuzzer target; it could not be configured because the native Clang
link environment lacks `msvcrtd.lib`/`oldnames.lib`. Deterministic property
stress is the executed fallback.

## Golden vectors and arithmetic

`tests/golden_vectors.txt` contains the frozen C/H/H2 empty, zero and maximum
cases. Its SHA-256 is recorded in `tests/golden_vectors.sha256`.
`docs/mathematics.md` defines the state domains, transformations, exact
composition equations, inverse limitations and intentional unsigned wrap.
All production arithmetic is unsigned. Rotations normalize counts, and
freestanding cross-compilation found no missing standard assumptions after the
explicit `<stddef.h>` fix.

## ESP32-S3 regression

Target: ESP32-S3 revision 0.2, 240 MHz, 16 MiB flash, 8 MiB PSRAM, ESP-IDF
v5.5.1, Xtensa GCC 14.2.0, COM37. The release-only harness links the three
production source files directly and uses internal RAM for all states.

Final clean run:

| Operation | Cycles/op | ns/op | State |
|---|---:|---:|---:|
| C update | 150 | 625 | 32 B |
| C update + inverse | 314 | 1311 | 32 B |
| H update | 293 | 1221 | 64 B |
| H combine | 150 | 625 | 64 B |
| H2 update | 113 | 472 | 32 B |
| H2 combine | 146 | 611 | 32 B |

All release algebra tests, H2 windows 16–16384, 10,000 mutation cases and a
1,000,000-event stream passed. Internal heap was 326,268 bytes before and
after the run; PSRAM free was 8,386,192 bytes before and after. The harness
task high-water measurement was 1,768 bytes; this is task/harness stack, not a
claim that each primitive needs that much stack. No reset or watchdog error was
observed in the final subscribed-task run.

The final app image was 0x30c30 bytes. GCC `-O3` host object contributions
(text+data) were approximately C 980 B, H 2380 B and H2 1132 B; GCC `-Os`
was approximately C 612 B, H 640 B and H2 796 B. These are object sizes, not
whole firmware sizes.

## Release blockers

* `LICENSE` is absent. No license was invented during this audit, so a public
  v0.1.0 release cannot be legally positioned as ready.

## Verdict

`NOT_READY` for a public v0.1.0 release decision. The production core has no
known correctness defect from the executed audit, but the missing license is
an explicit release blocker. Sanitizer and native coverage-guided fuzz results
remain unavailable due to toolchain/environment limitations and are recorded,
not represented as passes.
