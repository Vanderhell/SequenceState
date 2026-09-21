#!/usr/bin/env python3
"""Exact composition, associativity, tree and inverse probe for candidate H."""
import json, random, time
from pathlib import Path
from research_batch import affine_state, affine_combine, MASK

def inverse(t):
    n=len(t)//2; aa=t[:n]; bb=t[n:]; ia=[pow(a,-1,1<<32) for a in aa]
    return tuple(ia)+tuple((-ia[i]*bb[i])&MASK for i in range(n))

def main():
    rng=random.Random(20260921); assoc=0; exact=0; inverse_ok=0; tree_ok=0
    for _ in range(10000):
        a=[rng.randrange(256) for _ in range(rng.randrange(20))]
        b=[rng.randrange(256) for _ in range(rng.randrange(20))]
        c=[rng.randrange(256) for _ in range(rng.randrange(20))]
        sa,sb,sc=map(affine_state,(a,b,c))
        if affine_combine(affine_combine(sa,sb),sc)==affine_combine(sa,affine_combine(sb,sc)): assoc+=1
        if affine_combine(sa,sb)==affine_state(a+b): exact+=1
        total=affine_combine(sa,sb)
        if affine_combine(total,inverse(sb))==sa: inverse_ok+=1
        chunks=[a,b,c]; tree=affine_combine(affine_combine(sa,sb),sc)
        if tree==affine_state(sum(chunks,[])): tree_ok+=1
    result={"cases":10000,"exact_concat":exact,"associative":assoc,"inverse_suffix":inverse_ok,"tree_reduction":tree_ok}
    path=Path(__file__).resolve().parents[1]/"research"/"composability_results.json"
    path.write_text(json.dumps(result,indent=2),encoding="utf-8")
    print(json.dumps(result))
if __name__=="__main__": main()
