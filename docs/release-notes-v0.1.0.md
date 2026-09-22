# SequenceState v0.1.0

SequenceState v0.1.0 provides three fixed-size deterministic C11 primitives:

- **Trace / C-1.0:** 32-byte ordered trace state with exact rollback when the
  removed event is known.
- **Compose / H-1.0:** 64-byte affine sequence state with exact associative
  ordered block composition and tree reduction.
- **Range / H2-1.0:** 32-byte composable range state with exact prefix removal,
  hierarchical summaries and sliding-window support.

The caller owns all state. The production core uses no heap, recursion or
mutable global state and has no operating-system dependency. The portable core
has been checked with GCC, Clang/clang-cl and MSVC, with additional Cortex-M0,
Cortex-M4 and RV32 compile checks.

ESP32-S3 validation at 240 MHz with GCC `-O3` measured 150 cycles per Trace
update, 293 per Compose update, 150 per Compose combine, 113 per Range update and
146 per Range combine. Range windows from 16 to 16384 events and a one-million-
event stream passed; PSRAM was not required.

These summaries are not lossless history storage and do not provide cryptographic
collision resistance, authentication or tamper resistance. See the public
headers and [`docs/mathematics.md`](mathematics.md) for exact API and algebraic
contracts.

Licensed under the Apache License 2.0. See [`LICENSE`](../LICENSE) and
[`NOTICE`](../NOTICE).
