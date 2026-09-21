# Research checkpoint (batch 1)

This is an intermediate evidence record, not a final project verdict.

## Candidate audit

| ID | Code behavior | Current assessment |
|---|---|---|
| A | ARX plus lane feedback | hash-like avalanche; no combine summary |
| B | per-lane odd multiply/add and cross-lane rotation | hash-like; no exact composition exposed |
| C | rotate/odd multiply/lane permutation | reversible local operations, but final state is still lossy |
| D | symbol-dependent XOR only | structurally degenerate; no state feedback |
| E | symbol-selected lane permutation/mix | order-sensitive but hash-like geometry |
| F | additive neighbor mixing | name suggested composability, code does not provide it |
| H | accumulated per-lane affine transformation | exact block composition, 512-bit state; weak generic queryability |

## Batch configuration

The deterministic Python harness used seeds 7, 19, 101 for the declared sweep, explicit task seeds, widths 64/256/1024 where applicable, 72 task rows, exhaustive binary enumeration through length 10, 10,000-sample collision probes, and long-range marker probes through length 4096. The harness output is `research_results.json`; CSV task records are in `research_summary.csv`.

## Evidence

- Exhaustive binary length 10: A/B/C/E/F each had 1024 unique states; D had 2 unique states and 1022 excess collisions.
- Random collision probe: D had 8 unique states from 10,000 alphabet-4 length-16 samples; A/B/C/E/F/H/VSA/position-Bloom/FNV had 10,000 unique samples in this probe.
- Balanced membership probe: 64/256-bit Bloom reached 0.9306 linear-probe accuracy; SequenceState candidates were at or below 0.625 in the same setup.
- Balanced pair-order probe: 256-bit position-aware Bloom reached 1.0; candidate E reached 0.7361.
- Balanced pattern probe: 256-bit position-aware Bloom reached 0.9306; VSA reached 0.6667; the candidates did not exceed 0.5556.
- One-replacement similarity correlation across 1–16 replacements: position-aware Bloom 0.9595, VSA 0.9161, candidate E 0.2631, candidates A/C/F/H approximately 0.03–0.06. This is a small synthetic probe, not a universal similarity claim.
- H affine composition was bit-exact for 1,000 deterministic random splits. Its state summary is composable because it stores the accumulated affine transform, not because a generic 256-bit final vector is composable.
- Long-range marker-bit probes for A–F/H/VSA were near chance by length 4096 in this linear-probe setup; no candidate showed a demonstrated long-range advantage.

## Regression evidence

Candidate D's failure is retained as a regression condition: the symbol-dependent XOR transform has no feedback from the existing state, so binary sequences collapse into a tiny set of symbol aggregates. The C core also now has a frozen golden vector for `[0,1,2,3,UINT32_MAX]`.

## Interpretation

The current evidence favors specialized position-aware Bloom/VSA representations for the measured order/pattern/similarity tasks. H provides a real exact-composition construction, but its 512-bit transformation summary has not yet shown a useful information-retention advantage. The original A–F family has not produced evidence of a general advantage over task-specific baselines.
