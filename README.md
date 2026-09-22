# SequenceState

SequenceState is a small portable C11 library for deterministic ordered-stream
summaries. Version 0.1.0 contains three deliberately different primitives:

* **Trace (C-1.0)** is a 256-bit reversible ordered trace state. It supports
  exact rollback when the removed symbol is known. It is not a lossless history
  store and is not cryptographic.
* **Compose (H-1.0)** is a 512-bit affine sequence summary. Independent block
  summaries combine exactly and associatively, which supports tree reduction
  and ordered chunk aggregation. It is not an integrity/authentication hash.
* **Range (H2-1.0)** is a 256-bit composable summary containing polynomial,
  sum, position-moment and length lanes. It supports exact prefix removal and
  fixed-memory sliding-window updates when the outgoing event summary is
  available. It is not a Bloom-filter replacement or general semantic memory.

All public state is caller-owned, fixed-size and heap-free. The public headers
are under `include/sequence_state/`. Functions return `0` on success and `-1`
for invalid pointer arguments, except `ss_range_remove_prefix`, which returns
`1` for success and `0` for an invalid/too-long prefix. Equality functions
return `0` for invalid pointers.

## Build and test

```text
cmake -S . -B build -G Ninja -DSS_BUILD_RESEARCH=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

The historical candidate and baseline programs are opt-in:

```text
cmake -S . -B build-research -G Ninja -DSS_BUILD_RESEARCH=ON
```

The ESP32 validation harness is separate under `platform/esp32/` and is not a
dependency of the portable library.

## Scope and limitations

The states are deterministic summaries. They cannot reconstruct arbitrary
input history, provide cryptographic collision resistance, authenticate data,
or replace raw logs. H2 range operations require the relevant prefix summary;
arbitrary range localization requires a hierarchy of stored block summaries.
The algorithm identifiers are versioned independently from the library and
must not change silently.
