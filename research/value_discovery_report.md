# SEQUENCESTATE VALUE DISCOVERY REPORT

This report records the application-oriented batch after the generic information experiments. Results are deterministic synthetic workload evidence, not claims about all embedded systems.

## 1. EXECUTIVE RESULT

There is partial practical value, but it is concentrated in algebraic stream processing rather than generic compact semantic memory.

Demonstrated value:

- H is a fixed 512-bit affine transformation summary with exact concatenation, associativity, tree reduction, suffix inverse, and exact sliding-window updates when the outgoing event/prefix state is available.
- N/H2 is a 256-bit exact-composable polynomial + sum + position-moment state. It also supports exact composition and sliding windows, and reached 100% threshold detection on the synthetic execution-trace workload.
- H2 hierarchical states locate a changed block among 16/64/256/1024 blocks with log2(N) state comparisons in the executed tree probe.
- Candidate C and N reached 1.0 synthetic execution-trace threshold accuracy; position-aware Bloom also reached 1.0 and remains the simpler task-specific baseline.

Not demonstrated:

- A general advantage over Bloom/VSA/hash baselines for membership, generic patterns, or long-range semantic queries.
- Cryptographic integrity, semantic understanding, or arbitrary history retrieval.

## 2. SURVIVING PRIMITIVES

| Primitive | Definition | State | Useful property | Limitation |
|---|---|---:|---|---|
| C | reversible local ARX/multiply/permutation state | 32 B | strong local mutation/order sensitivity; 1.0 trace probe | no combine, hash-like geometry |
| H | 8 affine maps `s -> a*s+b mod 2^32` | 64 B | exact associative composition and inverse | weak generic queryability |
| N/H2 | `(length, polynomial, sum, position moment) mod 2^64` | 32 B | exact composition, position-sensitive moments, range/window algebra | collision-prone polynomial summary; not integrity hash |
| position-Bloom | position-derived bit sets | 8/32 B | strongest pair/pattern/similarity task probe | task-specific, not a general algebra |
| VSA | bipolar symbol projection with position rotation/superposition | configurable | strong synthetic similarity geometry | more state/compute; no exact combine in this implementation |

A, B, E and F remain usable as cheap order-sensitive fingerprints but did not show a distinctive application advantage in this batch. D remains rejected for aggregate/XOR degeneracy.

## 3. APPLICATION MATRIX

| Application | Best primitive | Relevant baseline | Measured result | Verdict |
|---|---|---|---|---|
| sequence fingerprinting | C/H/N | FNV/CRC | exact stream identity and mutation distances; no collision-resistance claim | specialized keep |
| execution trace monitoring | C or N | FNV, position-Bloom | C/N 1.0 threshold accuracy; FNV 0.833; position-Bloom 1.0 | specialized keep |
| protocol trace monitoring | C/H/N | FNV, Bloom | all tested stateful candidates and FNV 1.0; Bloom 0.667 | no unique winner |
| change detection | H/N | FNV/Bloom | H mean mutation distance 220.6; N 91.8; distance is persistent but threshold quality is workload-dependent | useful signal, not standalone proof |
| chunk deduplication | C/H/N | FNV | exact duplicates have zero distance; mutations/reorders remain detectable | specialized keep |
| chunk composition | H/N | replay | exact 1000/1000 composition | accepted algebraically |
| range operations | H/N | replay | exact prefix/suffix recovery through inverse/metadata equations | accepted algebraically |
| sliding windows | H/N | full recomputation | exact windows 16/64/256/1024; 40 compositions vs 320–20,480 replay events in probe | strong specialized value |
| distributed aggregation | H/N | raw chunk transfer | states compose without replay when global order is known | algebraically useful |
| storage block signatures | H/N | CRC/FNV | fixed block states compose and compare without replay; cryptographic integrity not claimed | useful for ordered-log/index helpers |
| anomaly detection | C/N | counters/FNV | 1.0 on controlled trace mutations; not yet shown on noisy real-world traces | synthetic specialized keep |
| state synchronization | H2 tree | full replay | root-to-leaf mismatch localization in 4/6/8/10 comparisons | strong structural value |
| mismatch localization | H2 tree | reread all blocks | log2 block comparisons instead of all block reads in model | strong structural value |

## 4. H DEEP ANALYSIS

For H, each symbol defines an affine transform `T_x(s)=a_x*s+b_x` per lane. The state stores the composition of all transforms. For left block A and right block B:

`combine(A,B) = T_B o T_A`

with `a=a_B*a_A` and `b=a_B*b_A+b_B` modulo `2^32`.

Measured/proven:

- exact concatenation: 10,000/10,000 Python cases and C golden/unit cases
- associativity: 10,000/10,000
- tree reduction: 10,000/10,000
- suffix inverse: 10,000/10,000
- exact sliding-window update: 20 updates for each window 16/64/256/1024
- odd multipliers have modular inverses; no division or floating point is needed

Prefix removal requires composition order care because transforms are non-commutative. If `total=B o A`, then `B=total o inverse(A)`. The implementation and test use that order.

The outgoing event or prefix state must be known for a sliding window. H does not reconstruct the removed event from the fixed state. This is a material limitation.

## 5. NEW CANDIDATE N/H2

H2 stores four 64-bit quantities:

`length`, `P`, `sum`, `moment`

where `P` is an order-sensitive polynomial recurrence and `moment=Σ i*f(x_i)`. Concatenation is exact with known block lengths; the position moment shifts by `length(left)*sum(right)`. It is 32 bytes and supports exact prefix removal from `(total,prefix)`.

H2 is not claimed to be better than a rolling polynomial hash for collision resistance. Its value is the combination of exact composition, position moment, and fixed-size window/range algebra.

## 6. BASELINE COMPARISON

The application harness includes FNV, Bloom, position-Bloom, and the previous CRC/SUM/XOR/Count-Min/VSA baseline set. Position-Bloom remains stronger for generic pair/pattern/similarity probes. FNV remains a simpler stream fingerprint. H/H2 provide operations those baselines do not provide directly: exact block algebra, suffix/prefix removal, and tree range comparison.

## 7. EMBEDDED COST

- H state RAM: 64 bytes; 16 words touched per update; eight odd modular multiplications and additions.
- H2 state RAM: 32 bytes; polynomial multiply plus fixed modular additions; exact combine uses one modular exponentiation for arbitrary block length unless powers are cached.
- A–F state RAM: 32 bytes and fixed eight-lane integer work.
- Python harness does not measure MCU energy or compiled code size; no hardware target is available. These remain explicitly unmeasured rather than inferred.
- C H implementation is portable C11 candidate code with no heap, floating point, OS API, threads, or filesystem dependencies.

## 8. INFORMATION VALUE

Generic membership/pattern/similarity results still favor specialized baselines. The new value is operational: sequence order is preserved in a composable transform, not made directly queryable as arbitrary history. H2's moment lane adds position-sensitive aggregate information but is not lossless.

## 9. CLASSIFICATION

- C: `SPECIALIZED_KEEP` for cheap mutation/order-sensitive execution trace detection.
- H: `ACCEPTED` as a composable/invertible sequence algebra; not accepted as a universal memory.
- N/H2: `SPECIALIZED_KEEP` for 32-byte exact-composable range/window summaries.
- position-Bloom: `SPECIALIZED_KEEP` as a baseline for positional event queries, not SequenceState core.
- VSA: `SPECIALIZED_KEEP` for similarity-oriented experimental comparison.
- A/B/E/F: `REDUNDANT` or `WORSE_THAN_BASELINE` for the tested application workloads.
- D: `REJECTED` because of structural aggregate collapse.

## 10. FINAL PROJECT VERDICT

`SPECIALIZED VALUE`

SequenceState is not demonstrated as a generally superior fixed-size semantic sequence memory. It does contain practical algebraic primitives for exact chunk composition, hierarchical range comparison, inverse-aware streaming windows, and compact trace/change signatures. Those uses are narrow, but the advantages are measurable and distinct from Bloom/VSA membership or similarity tasks.

## 11. NEXT STEP

Implement a production-quality H2 range/window API with explicit metadata and benchmark it on a real target or cycle-accurate MCU toolchain against rolling polynomial hash and full-window replay.
