# Production primitive definitions

All arithmetic below is unsigned modular arithmetic. C uses words modulo
`2^32`; H uses eight affine lanes modulo `2^32`; H2 uses lanes modulo `2^64`.

## C-1.0 trace

The initial state is the fixed eight-word constant with lane zero XORed by
`0x00030003`. For symbol `x`, define

```text
k = mix(x + 3 * 0x9e3779b9)
t_i = rotl32(S_i XOR k, (7*i + x) mod 32) * 0x9e3779b1
```

The next state is `t` after the permutation `(0,3,6) <- (3,6,0)`. `mix` is
the two odd multiplication/shift mixer in `core/src/trace.c`. Every operation
is bijective: rotation is bijective, multiplication by the odd constant has a
modular inverse, and the lane permutation is bijective. The inverse API
therefore restores the prior state when the exact symbol is supplied:

```text
inverse_update(update(S, x), x) = S
```

This is known-event rollback, not unknown-history recovery.

## H-1.0 compose

Each lane represents an affine map `f(z) = a*z + b (mod 2^32)`. The empty
sequence is the identity `(1,0)`. An event creates an affine map `(m,o)` where
`m` is odd. Appending a right block after a left block is composition:

```text
(a,b) = (a_right*a_left,
         a_right*b_left + b_right)
```

Associativity follows from associativity of function composition. The identity
is the empty state. Since every multiplier is odd, each event map is bijective
and the inverse summary is exact.

## H2-1.0 range

For event values `v_i`, the state stores length `n`,

```text
P = v_0*r^(n-1) + ... + v_(n-1)
Q = sum(v_i)
M = sum(i*v_i)
```

where `r = 0x9e3779b185ebca87` and `v_i` is the documented mixer multiplied
by `0x100000001b3`, all modulo `2^64`. For left/right blocks:

```text
n = nL + nR
P = P_L*r^nR + P_R
Q = Q_L + Q_R
M = M_L + M_R + nL*Q_R
```

The implementation uses `uint64_t` wraparound intentionally. Length therefore
has a modulo-`2^64` limit; callers must keep logical sequence lengths below
that limit when length uniqueness matters. Removing a known prefix uses

```text
nR = nT - nP
P_R = P_T - P_P*r^nR
Q_R = Q_T - Q_P
M_R = M_T - M_P - nP*Q_R
```

All combine and prefix-removal operations are alias-safe for their output.
