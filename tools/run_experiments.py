#!/usr/bin/env python3
"""Deterministic lightweight information probes; outputs CSV-like records."""
import random, statistics
SEED=12345
def fnv(seq):
    v=2166136261
    for x in seq: v=((v^x)*16777619)&0xffffffff
    return v
def hamming(a,b): return (a^b).bit_count()
def main():
    rng=random.Random(SEED); rows=[]
    for n in (0,1,2,4,8,16,64,256,1024,4096,16384,65536,262144,1048576):
        base=[rng.randrange(256) for _ in range(n)]
        mutated=base[:]
        if mutated: mutated[n//2]^=1
        rows.append((n, fnv(base), fnv(mutated), hamming(fnv(base),fnv(mutated))))
    print("seed,length,state,mutated_state,hamming_1_symbol")
    for row in rows: print(*row,sep=",")
if __name__ == "__main__": main()
