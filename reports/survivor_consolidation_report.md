# SEQUENCESTATE SURVIVOR CONSOLIDATION REPORT

## C — STRONG TRACE

### Mathematical definition

For symbol-derived `k`:

`t_i = ROTL32(s_i XOR k, 7i+x) * 0x9e3779b1 mod 2^32`

followed by the fixed lane permutation `(0,3,6) <- (3,6,0)`. The multiplier is odd, rotations/XOR/permutation are bijective, and the inverse requires the same symbol.

### Guarantees and evidence

- `inverse(update(S,x),x)=S` is derived directly from odd multiplication and inverse permutation.
- Python: 100,000/100,000 forward/inverse roundtrips.
- C: 10,000/10,000 roundtrips under GCC, MSVC and Clang-cl.
- Compiled C profile: 1,000,000/1,000,000 roundtrips; 0.041 s GCC Release, 0.036 s MSVC Release, 0.034 s Clang-cl Release.
- State: 32 B, fixed working state, no heap.
- Operation profile: 8 32-bit multiplies, 8 XORs, 8 rotates, fixed permutation plus shared symbol mix.

### Trace and mutation results

The earlier easy trace achieved 1.0 threshold accuracy. A harder trace with legitimate optional branches/retries reduced canonical-reference threshold accuracy to 0.51, so C is not a general anomaly detector under uncontrolled variation. Its correct use is known-trace validation/rollback, not generic nearest-normal anomaly detection.

### Application

Reversible firmware/event trace state, speculative state-machine execution, and transaction rollback where the removed event is known.

### Limitation

No compact block composition was derived. Unknown-event removal is impossible from the fixed state alone.

### Verdict

`SPECIALIZED_KEEP`

## E — LIGHT TRACE

### Mathematical definition

The actual C loop is in-place and sequential:

`p=(x XOR k) mod 8`

`s_i' = ROTL32(s_(i+p mod 8) + k, i+p)`

where `s_(i+p)` may already have been updated during the same loop.

### Correctness correction

An earlier Python probe incorrectly used a snapshot of all lanes and reported E as reversible. The Clang-cl test exposed the discrepancy. The actual in-place E implementation has no valid general inverse API; the snapshot inverse was removed.

### Cost and results

- State: 32 B.
- Per-lane work: add + rotate; symbol mix is shared.
- Release benchmark for 1,000,000 updates: GCC 0.016 s, MSVC 0.015 s, Clang-cl 0.016 s.
- C benchmark C was 0.014/0.015/0.014 s respectively, so E has no measured cost advantage.
- Hard trace threshold accuracy: 0.5125.
- Periodic identity: 16/16 periods distinguished; no distinct-sequence collision in 20,000 random samples.

### Limitation

It is neither reversible as implemented nor faster than C on the available compilers. Its lower mutation-distance scale does not produce a measured application advantage.

### Verdict

`REDUNDANT`

## H — COMPOSE

### Mathematical definition

Each lane stores an affine map `s -> a*s+b (mod 2^32)`. A symbol composes an odd multiplier and offset into each lane. For blocks A and B:

`Combine(A,B) = T_B o T_A`

`a = a_B*a_A mod 2^32`

`b = a_B*b_A+b_B mod 2^32`

### Proof-level properties

Affine-map composition is associative, so:

`Combine(Combine(A,B),C)=Combine(A,Combine(B,C))`.

Odd multipliers have modular inverses. Therefore known prefix/suffix transforms can be removed by composition with the inverse in the correct non-commutative order.

### Evidence

- Exact concatenation: 10,000/10,000 consolidation cases.
- Associativity: 10,000/10,000.
- Prefix removal: 10,000/10,000.
- Tree reduction: 10,000/10,000 in prior and current tests.
- Exact sliding windows: sizes 1 through 16,384, all tested patterns passed.
- State: 64 B.
- Release update benchmark: GCC 0.021 s, MSVC 0.020 s, Clang-cl 0.018 s per 1,000,000 updates.
- Combine benchmark: GCC 0.004 s, MSVC 0.007 s, Clang-cl 0.006 s per 1,000,000 combines.

### Distributed and range role

Independent ordered chunks can transmit 64-byte states instead of raw event blocks when global ordering is already known. H is not a cryptographic integrity primitive. Range removal requires the relevant prefix/suffix state; arbitrary range extraction requires a hierarchy of stored summaries.

### Limitation

H carries no query-oriented moments and did not beat task-specific Bloom/VSA probes. It is algebra, not semantic memory.

### Verdict

`ACCEPTED`

## H2 — RANGE

### Mathematical definition

H2 stores four 64-bit values:

`(length, P, sum, moment)`

with `P' = P*R+f(x)`, `sum'=sum+f(x)`, and `moment'=moment+length*f(x)` modulo `2^64`. Concatenation uses:

`P_AB=P_A*R^len(B)+P_B`

`sum_AB=sum_A+sum_B`

`moment_AB=moment_A+moment_B+len(A)*sum_B`.

### Evidence

- Exact concatenation, associativity and prefix removal: 10,000/10,000.
- State: 32 B.
- Release update benchmark: 0.003 s per 1,000,000 updates under GCC, MSVC and Clang-cl.
- Exact sliding windows: sizes 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 and 16,384; all tested random, constant and periodic cases passed.
- H2 nearest-normal hard trace accuracy: 0.755, compared with C 0.520, E 0.535 and FNV 0.555.

### Hierarchical mismatch localization

One changed block was localized exactly for 16, 64, 256, 1024, 4096, 16,384 and 65,536 blocks using 4, 6, 8, 10, 12, 14 and 16 state comparisons respectively. This is empirical evidence consistent with `log2(N)` comparisons for the binary hierarchy. It is not cryptographic integrity.

### Application

32-byte append-only log/range summaries, sliding telemetry windows, ordered chunk indexes, replica synchronization and mismatch localization.

### Limitation

Polynomial collisions are possible; H2 must not replace CRC or cryptographic hashes for integrity. Sliding removal requires the outgoing event/prefix summary and does not reconstruct it from H2 alone.

### Verdict

`ACCEPTED`

## C VS E

They do not justify two production primitives in the current implementation. C is exactly reversible and has stronger trace behavior; E is neither reversible nor faster in Release. E should be removed from the promoted API or rewritten as a deliberately snapshot-based new version with new evidence. Under the frozen current definition, E is `REDUNDANT`.

## H VS H2

H is the minimal 64-byte affine transformation algebra with per-lane invertible maps and no query metadata. H2 is a smaller 32-byte structured summary with exact low-order content/position moments and better measured update cost. H is the cleaner general composition primitive; H2 is the range/window/index primitive. They occupy distinct algebraic roles.

## PARETO FRONT

- exact known-symbol rollback: C
- exact affine block composition and inverse transform: H
- smallest exact range/window/moment state: H2
- E owns no measured Pareto point after correcting its implementation model

## REALISTIC APPLICATION MAP

| Workload | Primitive | Baseline | Measured reason | Limitation |
|---|---|---|---|---|
| known execution rollback | C | replay/full trace | exact 1M C roundtrips, 32 B | removed event must be known |
| ordered chunk reduction | H | replay | exact associative combine, 64 B | no generic query semantics |
| telemetry sliding window | H2 | full O(N) recompute | exact windows through 16,384, 32 B | outgoing event/summary required |
| hierarchical log mismatch | H2 | raw block scan | 4–16 comparisons for 16–65,536 blocks | non-cryptographic |
| distributed ordered chunks | H | replay/raw transfer | fixed state transfer and exact combine | global order must be known |

## ROBUSTNESS

- Golden corpus: empty, boundary, repeated, alternating, ascending, descending, random and trace/protocol sequences; GCC/MSVC/Clang-cl bit-identical.
- C rollback: 1,000,000/1,000,000.
- H/H2 algebra: 10,000/10,000 exact tests in consolidation plus prior 10,000 tests.
- C/E/H/H2 small binary collision probes: no collision found in tested distinct sequences; prior D-only collapse remains separate.
- E repeated-zero stream showed a discovered cycle at start 125, length 56 in the Python finite-state probe; this further weakens E as a promoted primitive.
- Sanitizers remain unavailable because the installed MinGW runtime lacks ASan/UBSan libraries.

## EMBEDDED COST

- C: 32 B, 8 32-bit multiplies + XOR/rotate/permutation; measured inverse roundtrip in C.
- E: 32 B, add/rotate in lane loop, but no measured speed advantage and no inverse.
- H: 64 B, eight affine lanes; exact combine is fixed eight-lane arithmetic.
- H2: 32 B, 64-bit modular polynomial/moment arithmetic; fastest measured update, but 64-bit operation cost is target-dependent.
- No real MCU hardware, code-size, stack-size or energy measurements were available. MCU suitability beyond operation profiles is estimated, not measured.

## FINAL CLASSIFICATION

- C: `SPECIALIZED_KEEP`
- E: `REDUNDANT`
- H: `ACCEPTED`
- H2: `ACCEPTED`

## FINAL LIBRARY SHAPE

Expose three specialized primitives, not four:

- `ss_trace_reversible_*` for C
- `ss_compose_*` for H
- `ss_range_*` for H2

Do not promote the current E implementation. If a lightweight reversible trace API is still desired, it must be a separately versioned snapshot-based redesign, not an alias for current E.

## MOST IMPORTANT RESULT

The strongest demonstrated property is not generic information retention: it is the separation of exact algebraic roles. C provides real known-event rollback; H provides minimal exact affine composition; H2 provides a smaller exact range/window/moment state. E does not provide a fourth defensible role under its actual implementation.

## NEXT STEP

Promote C, H and H2 into separate documented C APIs and run the same benchmark/correctness suite on a real Cortex-M or RV32 target; do not promote current E.
