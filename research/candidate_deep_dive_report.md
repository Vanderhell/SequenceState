# SEQUENCESTATE CANDIDATE DEEP-DIVE REPORT

The experiments in `candidate_deep_dive.json` are property-matched: mutation signatures, trace detection, inverse roundtrips, periodic streams, and adversarial distinct-sequence collisions. Generic membership results are not used as the primary rejection criterion here.

## CANDIDATE A

### Math

For lanes `i=0..7`, with `k=mix(x+0x9e3779b9*(id+1))`:

`s_i' = ROTL(s_i + k + i*C_i, x+3i) XOR s_(i+3 mod 8)`

The update is sequential, so wrapped lane references may observe already updated lanes. It uses state feedback and non-commuting order-sensitive operations. No inverse or block-combine representation was derived.

### Unique properties

- order-sensitive: yes
- reversible: not established; cyclic lane dependencies prevent the simple local inverse available in C/E
- composition: no exact compact summary
- diffusion: near-avalanche
- similarity: weak; mutation distances are almost independent of edit type
- update: 32 B; approximately 16 adds, 8 XORs, 8 rotates plus shared mix

### Targeted results

- 32 B trace threshold accuracy: `0.835`
- mean Hamming distance for single/adjacent/distant/missing/duplicate/block-reverse mutations: `129.03/127.78/127.64/127.89/127.92/128.53`
- periodic states: `16/16` unique periods 1–16
- 20,000 distinct random adversarial samples: no distinct-sequence collision found

### Application and limitation

A is a conventional low-cost order-sensitive stream fingerprint. It is useful only where a cheap change signal is sufficient; C has the stronger trace result and E has lower arithmetic cost. No measured A-specific advantage remains.

### Classification

`REDUNDANT` — dominated by C/E for the tested specialized uses and by FNV/CRC for simple fingerprinting.

## CANDIDATE B

### Math

`s_i' = (s_i * ((k|1)+2i) + ROTL(s_(i+1),11)) XOR k`

The next-lane dependency is sequential and cyclic. Multiplication is odd only for `k|1` before adding `2i`, so the effective multiplier can be even for some lanes. There is cross-lane coupling, but no derived inverse or combine operation.

### Unique properties

- order-sensitive: yes
- reversible: not established; effective lane multipliers are not uniformly odd
- diffusion: strong, hash-like
- collision behavior: no distinct collision in 20,000 random samples
- update: 32 B; 8 multiplies, 8 adds, 8 rotates, 8 XORs plus shared mix

### Targeted results

- trace threshold accuracy: `0.835`
- mean mutation distances: `127.92/127.89/127.89/128.53/127.64/127.74`
- periodic states: `16/16`
- no distinct-sequence collision found in 20,000 samples

### Application and limitation

B is a more expensive hash-like fingerprint with no demonstrated advantage over A/FNV/CRC. Cross-lane multiplication did not improve trace detection or adversarial behavior in this batch.

### Classification

`REDUNDANT` — no measured property justifies its multiply cost.

## CANDIDATE C

### Math

First apply independent per-lane maps:

`t_i = ROTL(s_i XOR k, 7i+x) * 0x9e3779b1 mod 2^32`

then permute lanes `0,3,6` as `(t_3,t_6,t_0)`. The multiplier is odd and every operation is bijective for known `x`.

### Unique properties

- order-sensitive: yes
- reversible: yes, exact known-symbol inverse
- composition: no compact block summary
- locality: each lane is independently invertible before the final fixed permutation
- diffusion: strong
- update: 32 B; 8 multiplies, 8 XORs, 8 rotates plus one fixed lane permutation

### Targeted results

- Python forward/inverse roundtrip: `100,000/100,000`
- C forward/inverse roundtrip: `10,000/10,000` under GCC, MSVC, and Clang-cl
- trace threshold accuracy: `1.000`
- mean mutation distances: `127.72/127.65/127.15/128.09/128.99/129.31`
- periodic states: `16/16`
- no distinct-sequence collision found in 20,000 samples
- C core tests and golden vectors remain passing

### Application

`SPECIALIZED_KEEP` for reversible execution-trace and speculative state-machine signatures. A device can apply a known event, validate a trace change, and undo the event exactly without storing a growing state.

Measured advantage: 32 B state, exact inverse, 1.0 controlled trace detection, and fixed eight-lane integer work. The relevant competitor is full trace replay or H/H2 when composition is more important than rollback cost.

### Limitation

The removed symbol must be known. C has no exact compact inverse for an unknown block, and the final state is hash-like for similarity.

## CANDIDATE D

### Math

`s_i' = s_i XOR ROTL(k+i*C,4i)`

The previous state is never read on the right-hand side except for the XOR with the lane itself. Therefore the final state is an XOR aggregate of symbol-derived lane constants, not a state-dependent sequence transform.

### Targeted results

- exhaustive binary length 10: only `2` unique states out of `1024`
- 20,000 random alphabet-4 length-16 samples: a distinct-sequence collision found
- periodic states 1–16: only `7` unique states
- adjacent swap, distant swap, and block reverse: mean state distance `0`

### Classification

`REJECTED` — structural collapse is proven and retained as a regression.

## CANDIDATE E

### Math

Let `p=(x XOR k) mod 8`. Snapshot old lanes and compute:

`s_i' = ROTL(old_s_(i+p mod 8) + k, i+p)`

This is a symbol-selected lane permutation followed by independent add/rotate maps. Each update is bijective for known `x`, but the accumulated final state has no compact block summary.

### Unique properties

- order-sensitive: yes
- reversible: yes; Python roundtrip `100,000/100,000`
- composition: no compact summary
- locality: permutation-selected; no multiply in lane update
- similarity: lower avalanche than C/A/B/F, but no useful metric correlation established
- update: 32 B; 8 adds, 8 rotates, lane snapshot/permutation plus shared mix

### Targeted results

- trace threshold accuracy: `0.835`
- lowest mutation distances among A–F: single `109.48`, adjacent swap `106.89`, distant swap `122.08`, missing `100.86`, duplicate `110.08`, block reverse `106.26`
- periodic states: `16/16`
- no distinct-sequence collision found in 20,000 samples

### Application

`SPECIALIZED_KEEP` as a low-cost reversible permutation/order fingerprint when lower avalanche and zero per-lane multiply are desirable. It is a plausible MCU-friendly trace signature, but C is the stronger measured detector.

### Limitation

No exact block composition, no demonstrated query advantage, and no proof that the lower mutation distance improves a real threshold detector.

## CANDIDATE F

### Math

`s_i' = s_i + ROTL(k XOR s_(i+1 mod 8),3i)`

The update is sequential with a cyclic neighbor dependency. The final lane depends on updated lane 0, so a simple triangular inverse does not exist. The name “composable” is not supported by the implementation.

### Targeted results

- trace threshold accuracy: `0.835`
- mean mutation distances: `127.05/127.36/127.22/128.36/128.47/128.93`
- periodic states: `16/16`
- no distinct-sequence collision found in 20,000 samples
- no exact block composition or inverse derived

### Classification

`REDUNDANT` — E is cheaper and invertible; C is invertible with stronger trace detection; H/H2 are actually composable.

## PARETO FRONT

- C: exact known-symbol inverse, best trace score among A–F, 32 B.
- E: invertible, no lane multiplies, 32 B, lowest mutation-distance scale among A–F.
- H/H2: exact block composition, associative tree reduction, range/window algebra.

The front is property-specific rather than one total ranking. A/B/F are dominated for the tested dimensions; D is rejected.

## REDUNDANT CANDIDATES

- A: dominated by C/E on inverse or cost and by FNV/CRC for simple fingerprinting.
- B: multiplication cost without measured trace/collision/periodic advantage.
- F: no composition despite its name; dominated by C/E/H2.

## SPECIALIZED APPLICATION MAP

| Application | Candidate | Baseline | Measured evidence | Limitation |
|---|---|---|---|---|
| reversible execution trace | C | replay/full trace | 100,000 Python and 10,000 C inverse roundtrips; 1.0 trace threshold | known event required for rollback |
| low-cost permutation trace fingerprint | E | CRC/FNV | 32 B, 8 add/rotate lane operations, 16/16 periodic identity | detection score only 0.835 |
| generic cheap fingerprint | none of A–F | CRC/FNV | no A–F advantage | use simpler established baseline |
| collapse/invariant research control | D | none | explicit 2-state binary collapse | rejected |

## MCU SUITABILITY

| Candidate | Likely target | Reason | Caveat |
|---|---|---|---|
| C | Cortex-M4/M7, ESP32, RV32IM | fixed 32-bit multiply/rotate/XOR; exact inverse | multiply cost on M0/RV32I without M extension |
| E | Cortex-M0/M3/M4, RV32I | no per-lane multiply; add/rotate/permutation | branch/symbol-dependent permutation; weaker detection |
| A/F | small MCUs | integer-only and fixed 32 B | no unique value over E/CRC/FNV |
| B | M4/M7/ESP32 | cross-lane multiplies are available | cost not justified by measured evidence |
| D | any | cheap | structurally unusable |

No hardware cycle or code-size measurements were available. These are operation-informed suitability estimates, not measured MCU claims.

## FINAL CLASSIFICATION

- A: `REDUNDANT`
- B: `REDUNDANT`
- C: `SPECIALIZED_KEEP`
- D: `REJECTED`
- E: `SPECIALIZED_KEEP`
- F: `REDUNDANT`

## MOST IMPORTANT FINDING

Candidate C deserves to survive independently of H/H2: it is a genuinely bijective known-symbol state update, has a tested C inverse, and achieved perfect controlled execution-trace detection with a 32-byte state. Candidate E is a lower-arithmetic-cost reversible alternative, but its detection result was weaker. Neither replaces H/H2 composition.

## NEXT STEP

Benchmark C and E on an actual Cortex-M or cycle-accurate RV32 target with forward/inverse trace workloads, comparing 32-byte state cost against CRC/FNV replay and H2 window updates.
