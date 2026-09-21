# SEQUENCESTATE RESEARCH VERDICT

## ENVIRONMENT

- repository: `C:\Users\vande\Desktop\SequenceState`
- branch: `master`, local branch is 4 commits ahead of `origin/master`; no push performed
- compiler matrix: MSVC 19.42 Debug, GCC 16.1 Release, Clang-cl 19.1.5 Release
- test environment: Windows x64, Python 3.11, NumPy available; scikit-learn unavailable
- all three C compiler test matrices passed; core forbidden-API scan passed

## IMPLEMENTATION

- surviving research candidates: A, B, C, E, F as order-sensitive fixed-word transforms; H as a 512-bit affine transformation summary
- new candidate created: H, exact affine transformation composition
- rejected: D, due structural XOR degeneracy
- baselines: XOR, SUM, CRC32, CRC64, FNV-like, polynomial rolling, Bloom, Count-Min Sketch, position-aware Bloom, binary VSA/HDC
- experiments: deterministic fixed-width survey 64–4096 bits, balanced task probes, exhaustive binary enumeration, collision search, similarity geometry, long-range profile to 1,048,576 symbols, composition/tree/inverse tests

## BEST GENERAL CANDIDATE

- mathematical definition: each lane stores an affine map `s -> a*s+b (mod 2^32)`; a symbol applies an odd multiplier and symbol-derived offset; block composition is `(A2,B2) o (A1,B1) = (A2*A1, A2*B1+B2)`
- state size: 512 bits / 64 bytes
- update cost: 8 modular multiply/add updates
- strongest measured properties: exact concatenation, exact associativity, exact balanced-tree reduction, and exact suffix inverse, all 10,000/10,000 cases
- weakest measured properties: no measured membership, pattern, or long-range information advantage; affine summary behaves hash-like for similarity

## BEST SPECIALIZED CANDIDATES

- task: membership; candidate: 64/256-bit Bloom; advantage: 0.9306 linear-probe accuracy; limitation: does not encode relative order
- task: pair order; candidate: 256-bit position-aware Bloom; advantage: 1.0000; limitation: specialized positional bit budget, not a general sequence representation
- task: pattern; candidate: 256-bit position-aware Bloom; advantage: 0.9306; limitation: task-specific and collision-prone outside its probe
- task: similarity; candidate: 256-bit position-aware Bloom and 256-bit VSA; advantage: mutation-distance correlations 0.9595 and 0.9161; limitation: synthetic task and no universal semantic similarity claim

## INFORMATION RETENTION

- order: candidate E reached 0.7361 on the balanced pair-order probe; position-aware Bloom reached 1.0000
- membership: Bloom reached 0.9306; SequenceState candidates were at or below 0.625
- pair order: no SequenceState candidate exceeded position-aware Bloom
- pattern: VSA reached 0.6667; candidates did not exceed 0.5556; position-aware Bloom reached 0.9306
- similarity: A/C/F/H correlations were approximately 0.03–0.06; E was 0.2631; VSA was 0.9161; position-aware Bloom was 0.9595
- long range: learned marker-bit probes for A–F/H/VSA were near chance by length 4096; compiled marker perturbations remained nonzero through 1,048,576 symbols, which proves persistence of a difference but not decodability
- position: position-aware Bloom retained the strongest measured position signal; no general candidate matched it
- capacity degradation: D collapses to 2 states for exhaustive binary sequences of length 10; other candidates had 1024/1024 unique states in that small exhaustive space

## COLLISION / ADVERSARIAL

- exact collisions: exhaustive binary length 10 found 1022 excess collisions for D; A/B/C/E/F had none in that space
- random structural collision probe: 10,000 alphabet-4 length-16 samples produced only 8 unique D states; A/B/C/E/F/H/VSA/position-Bloom/FNV produced 10,000 unique samples
- cycle behavior: no short-cycle search found for A/B/C/E/F in the executed probes; D's aggregate behavior is the dominant degeneracy
- degeneracies: D has no state feedback and therefore reduces to a symbol aggregate; A/B/C/E/F/H show near-avalanche, hash-like distances rather than useful similarity geometry
- regression tests added: candidate-D exhaustive uniqueness regression and C golden vector `[0,1,2,3,UINT32_MAX]`

## COMPOSABILITY

- exact/approximate: H is exact
- combine equation: `T(B o A)` from affine summaries `A` and `B`, with `A_total=A_B*A_A`, `B_total=A_B*B_A+B_B mod 2^32`
- metadata required: none beyond the fixed 512-bit transformation summary
- associativity: 10,000/10,000 exact
- inverse/prefix/suffix capability: inverse suffix recovery 10,000/10,000; prefix/suffix replacement follows from affine inversion
- tree reduction: 10,000/10,000 exact
- A–F: no valid combine operation was derived from their stored final vectors

## ML PROBE

- tasks: balanced membership, pair order, and length-3 pattern detection
- decoder: frozen-state linear ridge probe, with `state_bits + 1` parameters; no large model and no training leakage between generated train/test partitions
- best results: Bloom membership 0.9306, position-aware Bloom pair order 1.0000, position-aware Bloom pattern 0.9306, VSA pattern 0.6667
- conclusion: the decoder probe measures task exposure, not proof of intrinsic semantics; SequenceState did not dominate the relevant baselines

## BASELINE COMPARISON

The relevant task comparisons at 64/256 bits were:

| Task | SequenceState result | Baseline result | State bytes |
|---|---:|---:|---:|
| membership | <= 0.625 | Bloom 0.9306 | 8/32 |
| pair order | E 0.7361 | position-Bloom 1.0000 | 32 |
| pattern | candidates <= 0.5556 | position-Bloom 0.9306; VSA 0.6667 | 32 |
| similarity correlation | E 0.2631 | position-Bloom 0.9595; VSA 0.9161 | 32 |
| exact composition | H 1.0000 | not supplied by Bloom/FNV | 64 |

## PERFORMANCE

- A–F smoke benchmark: approximately 55.6–71.4 million updates/s in GCC Release on this Windows host; MSVC Debug was approximately 14.9–18.9 million updates/s
- state bytes: A–F 32; H 64; Bloom/VSA figures above use their declared fixed budgets
- long-range C profile: all A–F processed through 1,048,576 symbols; GCC and MSVC produced identical output rows
- combine throughput: not separately timed; H combine is fixed eight-lane arithmetic

## ROBUSTNESS

- deterministic: PASS
- golden vectors: PASS under MSVC, GCC, and Clang-cl
- cross-compiler: PASS under all available C compilers
- sanitizers/tools: MinGW ASan/UBSan configuration could not link because the installed runtime lacked `libasan` and `libubsan`; this is recorded as unavailable, not PASS
- core restrictions: no heap, floating point, OS API, threads, filesystem, or network references found in `core/`

## ACTUAL DISCOVERY

The original A–F fixed recurrent states did not demonstrate a useful information-retention advantage over task-specific baselines. The strongest positive result is narrower: a fixed affine transformation summary can be exactly associative, tree-reducible, and invertible for suffix removal. That construction is algebraically useful, but its 512-bit summary did not show better membership, order, pattern, similarity, or long-range queryability than the specialized Bloom/VSA baselines.

## FINAL VERDICT

PARTIALLY

## WHY

SequenceState contains a genuinely useful composable state model (H), but the central claim of better retained ordered-sequence structure per byte/operation is not supported by the measured A–F candidates. Specialized baselines were consistently better on the tested information tasks. The project therefore survives as an algebra/composability result, not yet as a superior general sequence representation.

## NEXT STEP

Build a specialized queryable affine/multi-resolution state around H and test it against the position-aware Bloom/VSA baselines on held-out long-range pair and pattern tasks, retaining exact composition as a hard constraint.
