#!/usr/bin/env python3
"""Application-oriented, deterministic value discovery for SequenceState.

The measurements here use direct state distances and exact algebraic checks;
they do not assume that generic ML query accuracy is the only useful metric.
"""
import json, math, random, statistics, time
from pathlib import Path
from research_batch import encode, hamming, affine_state, affine_combine, MASK, mix

MASK64=(1<<64)-1
R64=0x9E3779B185EBCA87

def h2_update(s,x):
    n,p,total,moment=s; v=((mix(x)*0x100000001B3)&MASK64)
    return (n+1,(p*R64+v)&MASK64,(total+v)&MASK64,(moment+n*v)&MASK64)

def h2_state(seq):
    s=(0,0,0,0)
    for x in seq: s=h2_update(s,x)
    return s

def h2_combine(left,right):
    nl,pl,sl,ml=left; nr,pr,sr,mr=right
    return (nl+nr,(pl*pow(R64,nr,1<<64)+pr)&MASK64,(sl+sr)&MASK64,(ml+mr+nl*sr)&MASK64)

def h2_remove_prefix(total,prefix):
    n,p,s,m=total; l,pl,sl,ml=prefix; r=n-l
    sr=(s-sl)&MASK64; pr=(p-(pl*pow(R64,r,1<<64)))&MASK64
    mr=(m-ml-l*sr)&MASK64
    return (r,pr,sr,mr)

def inverse_affine(t):
    n=len(t)//2; aa=t[:n]; bb=t[n:]; inv=[pow(a,-1,1<<32) for a in aa]
    return tuple(inv)+tuple((-inv[i]*bb[i])&MASK for i in range(n))

def h_remove_prefix(total,prefix):
    # affine_combine(left,right) means right o left; total o inverse(prefix)
    # therefore places inverse(prefix) as the left argument.
    return affine_combine(inverse_affine(prefix),total)

def enc(seq,name):
    if name=="H": return affine_state(seq)
    if name=="N": return h2_state(seq)
    return encode(seq,256,name)

def dist(a,b,name):
    return hamming(a,b,512 if name=="H" else 256 if name not in ("N",) else 256)

def best_threshold(scores):
    points=sorted(set(x[0] for x in scores)); best=(0.0,None)
    for t in points:
        correct=sum((score>=t)==bool(label) for score,label in scores)
        if correct/len(scores)>best[0]: best=(correct/len(scores),t)
    return {"accuracy":best[0],"threshold":best[1]}

def candidate_names(): return ["cand0","cand1","cand2","cand4","cand5","H","N","fnv","bloom","pos_bloom"]

def trace_workload():
    rows=[]; base=[1,2,3,4,2,3,5]*8
    for name in candidate_names():
        ref=enc(base,name); scored=[]
        for seed in range(120):
            rng=random.Random(seed); seq=base[:]
            # Normal traces vary only within the declared machine-cycle choices.
            if seed%3==0: seq[7]=6
            abnormal=seed%2==0
            if abnormal:
                kind=seed%3
                if kind==0: seq[20]=99
                elif kind==1: seq[20:22]=seq[21:20:-1]
                else: seq.insert(24,77)
            scored.append((dist(enc(seq,name),ref,name),int(abnormal)))
        r=best_threshold(scored); rows.append({"application":"execution_trace","algorithm":name,**r,"normal_mean":statistics.mean(x for x,y in scored if not y),"abnormal_mean":statistics.mean(x for x,y in scored if y)})
    return rows

def protocol_workload():
    rows=[]; base=[10,12,11,10,12,11,10,12,11]*6
    for name in candidate_names():
        ref=enc(base,name); scored=[]
        for seed in range(120):
            seq=base[:]; abnormal=seed%2==0
            if abnormal:
                if seed%3==0: seq.remove(11)
                elif seed%3==1: seq.insert(18,13); seq.insert(19,14)
                else: seq[7:9]=seq[8:7:-1]
            scored.append((dist(enc(seq,name),ref,name),int(abnormal)))
        rows.append({"application":"protocol_trace","algorithm":name,**best_threshold(scored)})
    return rows

def change_detection():
    rows=[]
    for name in candidate_names():
        for mutation in ("single","burst","reorder","duplicate","delete"):
            base=[(i*17+3)%256 for i in range(64)]; seq=base[:]
            if mutation=="single": seq[31]^=1
            elif mutation=="burst": seq[20:24]=[250,251,252,253]
            elif mutation=="reorder": seq[10:14]=reversed(seq[10:14])
            elif mutation=="duplicate": seq[32:32]=seq[32:35]
            else: del seq[32]
            rows.append({"application":"change_detection","algorithm":name,"mutation":mutation,"distance":dist(enc(base,name),enc(seq,name),name)})
    return rows

def exact_composition():
    rows=[]; rng=random.Random(20260921)
    for name in ("H","N"):
        exact=assoc=tree=window=0; cases=1000
        for _ in range(cases):
            seq=[rng.randrange(65536) for _ in range(rng.randrange(0,128))]; cut=rng.randrange(len(seq)+1); a,b=seq[:cut],seq[cut:]
            if name=="H": sa,sb,whole=affine_state(a),affine_state(b),affine_state(seq); combined=affine_combine(sa,sb)
            else: sa,sb,whole=h2_state(a),h2_state(b),h2_state(seq); combined=h2_combine(sa,sb)
            exact += int(combined==whole)
            c=[seq[:len(seq)//3],seq[len(seq)//3:2*len(seq)//3],seq[2*len(seq)//3:]]
            sc=[h2_state(x) if name=="N" else affine_state(x) for x in c]
            if name=="H": assoc += int(affine_combine(affine_combine(sc[0],sc[1]),sc[2])==affine_combine(sc[0],affine_combine(sc[1],sc[2])))
            else: assoc += int(h2_combine(h2_combine(sc[0],sc[1]),sc[2])==h2_combine(sc[0],h2_combine(sc[1],sc[2])))
        rows.append({"algorithm":name,"exact_concat":exact/cases,"associative":assoc/cases,"state_bytes":64 if name=="H" else 32})
    return rows

def sliding_windows():
    rows=[]; rng=random.Random(88)
    for name in ("H","N"):
        for width in (16,64,256,1024):
            stream=[rng.randrange(65536) for _ in range(width+20)]; current=stream[:width]; ok=True
            for i in range(20):
                old=current[0]; new=stream[width+i]; prefix=enc([old],name); total=enc(current,name)
                reduced=h_remove_prefix(total,prefix) if name=="H" else h2_remove_prefix(total,prefix)
                appended=enc([new],name); current=current[1:]+[new]
                next_state=affine_combine(reduced,appended) if name=="H" else h2_combine(reduced,appended)
                ok &= next_state==enc(current,name)
            rows.append({"algorithm":name,"window":width,"exact":ok,"updates":20,"recompute_events":20*width,"incremental_compositions":40})
    return rows

def localization():
    rows=[]; rng=random.Random(123)
    for blocks in (16,64,256,1024):
        source=[[rng.randrange(256) for _ in range(64)] for _ in range(blocks)]; changed=[x[:] for x in source]; changed[blocks//2][3]^=1
        compared=0; lo=0; hi=blocks
        while hi-lo>1:
            mid=(lo+hi)//2; compared+=1
            left1=left2=(0,0,0,0)
            for x in source[lo:mid]: left1=h2_combine(left1,h2_state(x))
            for x in changed[lo:mid]: left2=h2_combine(left2,h2_state(x))
            if left1==left2: lo=mid
            else: hi=mid
        rows.append({"application":"tree_mismatch_localization","blocks":blocks,"state_comparisons":compared,"identified_block":lo,"raw_block_reads_if_replay":blocks})
    return rows

def dedup_workload():
    rows=[]; rng=random.Random(777)
    for name in candidate_names():
        same=[]; mutation=[]; reorder=[]
        for _ in range(100):
            base=[rng.randrange(65536) for _ in range(64)]; copy=base[:]; changed=base[:]; changed[17]^=1; swapped=base[:]; swapped[10:14]=reversed(swapped[10:14])
            same.append(dist(enc(base,name),enc(copy,name),name)); mutation.append(dist(enc(base,name),enc(changed,name),name)); reorder.append(dist(enc(base,name),enc(swapped,name),name))
        rows.append({"application":"chunk_deduplication","algorithm":name,"exact_duplicate_mean":statistics.mean(same),"one_mutation_mean":statistics.mean(mutation),"local_reorder_mean":statistics.mean(reorder)})
    return rows

def main():
    started=time.perf_counter(); result={"version":"value-discovery-1","property_matrix":{
        "A-F":"order-sensitive, fixed-memory, non-composable from final vector, hash-like distance",
        "D":"symbol aggregate, structurally degenerate","H":"exact associative affine transform, invertible suffix/prefix",
        "N":"exact polynomial/sum/position-moment composition, 32 bytes"}}
    result["execution_trace"]=trace_workload(); result["protocol_trace"]=protocol_workload(); result["change_detection"]=change_detection()
    result["composition"]=exact_composition(); result["sliding_windows"]=sliding_windows(); result["localization"]=localization()
    result["deduplication"]=dedup_workload()
    result["elapsed_seconds"]=round(time.perf_counter()-started,3)
    root=Path(__file__).resolve().parents[1]/"research"; root.mkdir(exist_ok=True)
    (root/"value_results.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
    print(json.dumps({"elapsed_seconds":result["elapsed_seconds"],"sections":list(result.keys())}))

if __name__=="__main__": main()
