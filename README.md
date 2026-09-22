# SequenceState

[![CI](https://github.com/Vanderhell/SequenceState/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/SequenceState/actions/workflows/ci.yml)
[![C11](https://img.shields.io/badge/C-C11-00599C.svg)](https://en.cppreference.com/w/c/language)
[![License](https://img.shields.io/badge/license-Apache--2.0-green.svg)](LICENSE)

Deterministic, fixed-size C11 primitives for ordered stream state.

| Primitive | State | Purpose |
| --- | ---: | --- |
| **Trace / C-1.0** | 32 B | Reversible ordered trace state with known-event rollback |
| **Compose / H-1.0** | 64 B | Exact associative composition of ordered blocks |
| **Range / H2-1.0** | 32 B | Composable ranges, prefix removal and sliding windows |

All production state is caller-owned, deterministic, bounded and heap-free.

## Why SequenceState

Use Trace for compact mutation-sensitive execution or protocol signatures when a
known event may need to be undone. Use Compose when independently processed
chunks must be reduced without replaying their events. Use Range when a fixed
summary must support exact prefix operations and hierarchical or sliding-window
workflows.

These are sequence summaries, not lossless storage. They are not cryptographic
hashes, authentication mechanisms, compression, Bloom filters or semantic memory.

## Quick start

The public headers are:

```c
#include <sequence_state/trace.h>
#include <sequence_state/compose.h>
#include <sequence_state/range.h>
```

Small working examples are available in:

- [`examples/example_trace_c.c`](examples/example_trace_c.c)
- [`examples/example_compose_h.c`](examples/example_compose_h.c)
- [`examples/example_sliding_h2.c`](examples/example_sliding_h2.c)

The API uses caller-allocated fixed-size state objects. Production functions do
not allocate memory or use mutable global state. See [`docs/mathematics.md`](docs/mathematics.md)
for the equations and [`include/sequence_state/`](include/sequence_state/) for
the contracts.

## Build and test

```text
cmake -S . -B build -G Ninja -DSS_BUILD_RESEARCH=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

The default build contains only the three production primitives. Historical
candidate and baseline programs are opt-in:

```text
cmake -S . -B build-research -G Ninja -DSS_BUILD_RESEARCH=ON
```

## Measured ESP32-S3 results

Final hardware validation used an ESP32-S3 N16R8 at 240 MHz with GCC `-O3`.
Timings are configuration-specific measurements, not portable cycle claims.

| Operation | Measured |
| --- | ---: |
| Trace update | 150 cycles |
| Trace forward + inverse | 314 cycles |
| Compose update | 293 cycles |
| Compose combine | 150 cycles |
| Range update | 113 cycles |
| Range combine | 146 cycles |

Range sliding windows from 16 through 16384 events passed. A one-million-event
stream passed with no heap drift; PSRAM was not required.

## Guarantees and limitations

- Trace provides exact rollback only when the removed symbol is known. It does
  not reconstruct unknown history.
- Compose provides exact ordered concatenation, an identity state and
  associative block composition suitable for tree reduction. It is not a
  cryptographic integrity primitive.
- Range provides exact composition and supported prefix/range/window operations
  when their required summaries or outgoing event information are available.
- Fixed-size states cannot losslessly represent arbitrary unbounded histories.
- Collision resistance and tamper resistance are not provided.
- Concurrent use of independent state objects is safe; callers must synchronize
  access to the same mutable state object.

## Portability and validation

The core is portable C11 and has been validated with GCC, Clang/clang-cl and
MSVC, plus cross-compilation checks for Cortex-M0, Cortex-M4 and RV32 targets.
The repository includes deterministic golden vectors, property tests, examples,
strict-warning builds and the ESP32-S3 hardware report.

Historical research material is under [`research/`](research/) and [`reports/`](reports/).
The current product truth is this README, the public headers and
[`docs/mathematics.md`](docs/mathematics.md).

## License

SequenceState is licensed under the Apache License 2.0. See [`LICENSE`](LICENSE)
and [`NOTICE`](NOTICE).
