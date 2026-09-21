#!/usr/bin/env python3
"""Property-matched deep dive for candidates A-F.

This intentionally measures trace/change/rollback/fingerprint behavior rather
than treating generic membership as the sole acceptance task.
"""
import json, random, statistics, time
from pathlib import Path
from research_batch import MASK, mix, rot, state, hamming, encode

INV_C=pow(0x9E3779B1,-1,1<<32)

def rotr(x,n): return rot(x,32-(n&31))

def candidate_c_update(words,x):
    k=mix(x+0x9E3779B9*3); old=[]
    for i,w in enumerate(words): old.append((rot(w^k,i*7+x)*0x9E3779B1)&MASK)
    old[0],old[3],old[6]=old[3],old[6],old[0]
    return old

def candidate_c_inverse(words,x):
    k=mix(x+0x9E3779B9*3); t=words[:]
    t[0],t[3],t[6]=words[6],words[0],words[3]
    return [rotr((t[i]*INV_C)&MASK,i*7+x)^k for i in range(len(t))]

def candidate_e_update(words,x):
    # Mirrors the actual C implementation: intentionally in-place and sequential.
    k=mix(x+0x9E3779B9*5); p=(x^k)&7
    for i in range(len(words)): words[i]=rot((words[(i+p)%len(words)]+k)&MASK,i+p)
    return words

def inverse_probe():
    rng=random.Random(20260921); rows=[]
    for name,forward,inverse in (("C",candidate_c_update,candidate_c_inverse),):
        passed=0; cases=100000
        for _ in range(cases):
            words=[rng.getrandbits(32) for _ in range(8)]; x=rng.getrandbits(32)
            passed += int(inverse(forward(words,x),x)==words)
        rows.append({"candidate":name,"cases":cases,"roundtrip_pass":passed})
    return rows

def mutation_probe():
    rng=random.Random(44); rows=[]
    mutations=("single","adjacent_swap","distant_swap","missing","duplicate","block_reverse")
    for kind in range(6):
        name=f"cand{kind}"; values={m:[] for m in mutations}
        for _ in range(200):
            base=[rng.randrange(65536) for _ in range(64)]
            variants={"single":base[:],"adjacent_swap":base[:],"distant_swap":base[:],"missing":base[1:],"duplicate":base[:],"block_reverse":base[:]}
            variants["single"][31]^=1; variants["adjacent_swap"][20],variants["adjacent_swap"][21]=variants["adjacent_swap"][21],variants["adjacent_swap"][20]
            variants["distant_swap"][3],variants["distant_swap"][55]=variants["distant_swap"][55],variants["distant_swap"][3]
            variants["duplicate"][31:31]=variants["duplicate"][31:32]; variants["block_reverse"][16:24]=reversed(variants["block_reverse"][16:24])
            ref=state(base,256,kind)
            for m in mutations: values[m].append(hamming(ref,state(variants[m],256,kind),256))
        rows.append({"algorithm":name,"mean_distance":{m:round(statistics.mean(v),2) for m,v in values.items()},"minimum_distance":{m:min(v) for m,v in values.items()}})
    return rows

def trace_threshold(kind):
    base=[1,2,3,4,2,3,5]*8; ref=state(base,256,kind); scored=[]
    for seed in range(200):
        seq=base[:]; abnormal=seed%2==0
        if seed%3==0: seq[7]=6
        if abnormal:
            if seed%3==0: seq[20]=99
            elif seed%3==1: seq[20:22]=seq[21:20:-1]
            else: seq.insert(24,77)
        scored.append((hamming(ref,state(seq,256,kind),256),int(abnormal)))
    best=0
    for threshold in sorted(set(x for x,_ in scored)):
        best=max(best,sum((x>=threshold)==bool(y) for x,y in scored)/len(scored))
    return best

def periodic_probe():
    rows=[]
    for kind in range(6):
        states=[]
        for period in range(1,17):
            seq=(list(range(period))*32)[:256]; states.append(state(seq,256,kind))
        rows.append({"algorithm":f"cand{kind}","unique_period_states":len(set(states)),"pairwise_distances":statistics.mean(hamming(states[i],states[i+1],256) for i in range(15))})
    return rows

def adversarial_probe():
    rng=random.Random(123); rows=[]
    for kind in range(6):
        seen={}; collision=None
        for _ in range(20000):
            seq=tuple(rng.randrange(4) for _ in range(16)); s=state(seq,256,kind)
            if s in seen and seen[s] != seq: collision={"first":seen[s],"second":seq}; break
            seen[s]=seq
        rows.append({"algorithm":f"cand{kind}","first_random_collision":collision})
    return rows

def cost_profile():
    # Counts are derived directly from the C implementation, excluding the shared symbol mix() cost.
    return [
        {"candidate":"A","state_bytes":32,"adds":16,"xors":8,"rotates":8,"multiplies":0,"branches":1,"notes":"lane feedback; common mix has 2 multiplies"},
        {"candidate":"B","state_bytes":32,"adds":8,"xors":8,"rotates":8,"multiplies":8,"branches":1,"notes":"cross-lane multiply"},
        {"candidate":"C","state_bytes":32,"adds":0,"xors":8,"rotates":8,"multiplies":8,"branches":1,"notes":"all local transforms bijective for known symbol"},
        {"candidate":"D","state_bytes":32,"adds":0,"xors":8,"rotates":8,"multiplies":0,"branches":1,"notes":"no state feedback; collapse"},
        {"candidate":"E","state_bytes":32,"adds":8,"xors":0,"rotates":8,"multiplies":0,"branches":1,"notes":"symbol-selected lane permutation"},
        {"candidate":"F","state_bytes":32,"adds":8,"xors":8,"rotates":8,"multiplies":0,"branches":1,"notes":"cyclic neighbor dependency"},
    ]

def main():
    started=time.perf_counter(); result={"version":"candidate-deep-dive-1","math":{
        "A":"s_i'=ROTL(s_i+k+i*c_i,r_i) XOR s_(i+3) sequentially",
        "B":"s_i'=(s_i*((k|1)+2i)+ROTL(s_(i+1),11)) XOR k sequentially",
        "C":"s_i'=ROTL(s_i XOR k,7i+x)*0x9e3779b1 followed by lane permutation",
        "D":"s_i'=s_i XOR ROTL(k+i*c,4i), independent of prior state",
        "E":"s_i'=ROTL(old_s_(i+p)+k,i+p), p=(x XOR k) mod lanes",
        "F":"s_i'=s_i+ROTL(k XOR s_(i+1),3i) sequentially with cyclic dependency"}}
    result["inverse"]=inverse_probe(); result["mutation"]=mutation_probe(); result["trace_accuracy"]={f"cand{k}":trace_threshold(k) for k in range(6)}
    result["periodic"]=periodic_probe(); result["adversarial"]=adversarial_probe(); result["cost"]=cost_profile(); result["elapsed_seconds"]=round(time.perf_counter()-started,3)
    root=Path(__file__).resolve().parents[1]/"research"; root.mkdir(exist_ok=True); (root/"candidate_deep_dive.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
    print(json.dumps({"elapsed_seconds":result["elapsed_seconds"],"inverse":result["inverse"],"trace_accuracy":result["trace_accuracy"]}))
if __name__=="__main__": main()
