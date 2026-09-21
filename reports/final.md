# SEQUENCESTATE FINAL VERDICT

Repository: C11 CMake project created in an initially empty non-Git directory
Branch: N/A
Starting commit: N/A (no Git metadata existed)
Final commit: N/A
Working tree: implementation and generated build directories present; no push performed

## SURVIVING CANDIDATES

- A-arx, B-cross-multiply, C-reversible, D-invariant, E-permutation, F-composable remain experimental candidates. No candidate is accepted by the available evidence.

## REJECTED CANDIDATES

- None formally rejected; the full information-retention campaign was not implemented before this milestone.

## BEST CANDIDATE

- name: C-reversible (fastest observed in the one-shot smoke run, not a quality verdict)
- state size: 256 bit / 32 bytes
- update cost: fixed eight-lane integer transform
- memory: fixed state only in core
- strongest property: deterministic order-sensitive mixing
- weakest property: no measured membership, pair-order, pattern, similarity, or combine advantage

## BASELINE COMPARISON

- XOR, SUM, FNV-like rolling, and CRC32 scalar baselines compile and are covered by a basic test.
- Bloom, Count-Min Sketch, and VSA/HDC are not yet implemented; therefore no superiority claim is justified.

## INFORMATION RETENTION

- order: basic candidate updates are sequence-dependent; no collision-rate matrix yet
- membership: not measured
- pair order: not measured
- patterns: not measured
- similarity: only a small deterministic FNV probe exists
- long range: not measured for candidates

## COMPOSABILITY

- combine: not implemented
- exact/approximate: not assessed
- associative: not assessed
- tree reduction: not assessed

## ROBUSTNESS

- deterministic: PASS in unit tests
- collision tests: only basic nontriviality check; insufficient for acceptance
- adversarial tests: not implemented
- sanitizers: unavailable in installed MinGW runtime
- cross-compiler: MSVC Debug, GCC Release, and Clang-cl Release PASS

## PERFORMANCE

One Windows MSVC Debug smoke run over 1,000,000 updates reported approximately 14.9–18.9 million updates/s across candidates A–F. This is a harness smoke result, not a portable benchmark conclusion.

## FINAL VERDICT

MODIFY

## WHY

- The fixed-size C11 foundation builds cleanly with strict warnings and passes the available tests on three compiler families.
- The research question is unresolved because the required information experiments, stronger baselines, combine analysis, and adversarial campaign are not yet present.

## NEXT TECHNICAL STEP

- Implement the deterministic collision/order/membership/pair-order/pattern experiment matrix and compare all six candidates against Bloom, Count-Min Sketch, and VSA/HDC baselines.
