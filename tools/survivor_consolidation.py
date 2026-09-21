#!/usr/bin/env python3
"""Final survivor positioning batch: hard traces, rollback, algebra, windows and indexes."""
import json, random, statistics, time
from pathlib import Path
from research_batch import state, update_words, affine_state, affine_combine, mix, hamming, baseline
from value_discovery import h2_state, h2_combine, h2_remove_prefix, h_remove_prefix
from candidate_deep_dive import candidate_c_update, candidate_c_inverse

def enc(seq,name):
    if name=="H": return affine_state(seq)
    if name=="H2": return h2_state(seq)
    if name=="FNV": return baseline(seq,256,"fnv")
    return state(seq,256,2 if name=="C" else 4)

def width(name): return 512 if name=="H" else 256
def d(a,b,name): return hamming(a,b,width(name))

def threshold(scores):
    return max(sum((s>=t)==bool(y) for s,y in scores)/len(scores) for t in sorted(set(s for s,_ in scores)))

def hard_trace():
    rows=[]
    for name in ("C","E","H","H2","FNV"):
        canonical=[1,2,3,4,5,6,7,8,9,10]*8; ref=enc(canonical,name); scores=[]
        for seed in range(400):
            rng=random.Random(seed); seq=[]
            for cycle in range(8):
                seq += [1,2,3,4]
                if rng.randrange(4)==0: seq += [11] # legal optional diagnostic branch
                seq += [5]
                if rng.randrange(3)!=0: seq += [6] # legal write omission
                seq += [7,8,9,10]
            abnormal=seed%2==0
            if abnormal:
                kind=seed%5
                if kind==0: seq.pop(20)
                elif kind==1: seq.insert(31,99)
                elif kind==2: seq[24:26]=seq[25:23:-1]
                elif kind==3: seq.insert(45,6)
                else: seq[52]=12
            scores.append((d(enc(seq,name),ref,name),int(abnormal)))
        rows.append({"algorithm":name,"accuracy":threshold(scores),"normal_mean":statistics.mean(s for s,y in scores if not y),"abnormal_mean":statistics.mean(s for s,y in scores if y)})
    return rows

def hard_trace_nearest():
    def make(seed, abnormal):
        rng=random.Random(seed); seq=[]
        for _ in range(8):
            seq += [1,2,3,4]
            if rng.randrange(4)==0: seq += [11]
            seq += [5]
            if rng.randrange(3)!=0: seq += [6]
            seq += [7,8,9,10]
        if abnormal:
            kind=seed%5
            if kind==0: seq.pop(20)
            elif kind==1: seq.insert(31,99)
            elif kind==2: seq[24:26]=seq[25:23:-1]
            elif kind==3: seq.insert(45,6)
            else: seq[52]=12
        return seq
    rows=[]
    for name in ("C","E","H","H2","FNV"):
        train=[enc(make(i,False),name) for i in range(100)]; scored=[]
        for i in range(100,200): scored.append((min(d(enc(make(i,False),name),x,name) for x in train),0))
        for i in range(200,300): scored.append((min(d(enc(make(i,True),name),x,name) for x in train),1))
        rows.append({"algorithm":name,"nearest_normal_accuracy":threshold(scored),"normal_mean":statistics.mean(x for x,y in scored if not y),"abnormal_mean":statistics.mean(x for x,y in scored if y)})
    return rows

def c_rollback():
    rng=random.Random(991); words=[rng.getrandbits(32) for _ in range(8)]; passed=0; cases=100000
    for i in range(cases):
        before=words[:]; x=rng.getrandbits(32); words=candidate_c_update(words,x); words=candidate_c_inverse(words,x); passed += int(words==before)
    multi=0
    for _ in range(100000):
        original=[rng.getrandbits(32) for _ in range(8)]; words=original[:]; symbols=[rng.getrandbits(32) for _ in range(8)]
        for x in symbols: words=candidate_c_update(words,x)
        for x in reversed(symbols): words=candidate_c_inverse(words,x)
        multi += int(words==original)
    return {"individual_cases":cases,"individual_pass":passed,"multi_event_cases":100000,"multi_event_pass":multi,"million_case_c_runtime":"see compiled ss_rollback_profile"}

def algebra():
    rng=random.Random(20260921); rows=[]
    for name in ("H","H2"):
        exact=assoc=remove=0; cases=10000
        for _ in range(cases):
            a=[rng.randrange(65536) for _ in range(rng.randrange(24))]; b=[rng.randrange(65536) for _ in range(rng.randrange(24))]; c=[rng.randrange(65536) for _ in range(rng.randrange(24))]
            if name=="H": sa,sb,sc=map(affine_state,(a,b,c)); whole=affine_state(a+b+c); combined=affine_combine(sa,sb); left=affine_combine(affine_combine(sa,sb),sc); right=affine_combine(sa,affine_combine(sb,sc)); remove_ok=h_remove_prefix(whole,sa)==affine_state(b+c)
            else: sa,sb,sc=map(h2_state,(a,b,c)); whole=h2_state(a+b+c); combined=h2_combine(sa,sb); left=h2_combine(h2_combine(sa,sb),sc); right=h2_combine(sa,h2_combine(sb,sc)); remove_ok=h2_remove_prefix(whole,sa)==h2_state(b+c)
            exact+=int(combined==h2_state(a+b) if name=="H2" else combined==affine_state(a+b)); assoc+=int(left==right); remove+=int(remove_ok)
        rows.append({"algorithm":name,"cases":cases,"exact_concat":exact,"associative":assoc,"prefix_remove":remove})
    return rows

def windows():
    rng=random.Random(808); rows=[]
    for name in ("H","H2"):
        for window in (1,2,4,8,16,32,64,128,256,512,1024,2048,4096,16384):
            passed=0; cases=6
            for mode in range(cases):
                stream=([mode%4]* (window+3)) if mode%3==0 else ((list(range(8))*(window+3))[:window+3] if mode%3==1 else [rng.randrange(65536) for _ in range(window+3)])
                current=stream[:window]
                for i in range(2):
                    prefix=enc([current[0]],name); total=enc(current,name); reduced=h_remove_prefix(total,prefix) if name=="H" else h2_remove_prefix(total,prefix); current=current[1:]+[stream[window+i]]; actual=affine_combine(reduced,enc([current[-1]],name)) if name=="H" else h2_combine(reduced,enc([current[-1]],name)); passed+=int(actual==enc(current,name))
            rows.append({"algorithm":name,"window":window,"cases":cases*2,"passed":passed})
    return rows

def hierarchy():
    rows=[]; rng=random.Random(73)
    for blocks in (16,64,256,1024,4096,16384,65536):
        leaves=[h2_state([(i*17)&255,(i*31+1)&255]) for i in range(blocks)]; changed=leaves[:]; changed[blocks//2]=h2_state([99,100]); levels_a=[leaves]; levels_b=[changed]
        while len(levels_a[-1])>1:
            aa=levels_a[-1]; bb=levels_b[-1]; levels_a.append([h2_combine(aa[i],aa[i+1]) for i in range(0,len(aa),2)]); levels_b.append([h2_combine(bb[i],bb[i+1]) for i in range(0,len(bb),2)])
        idx=0; comparisons=0
        for level in range(len(levels_a)-1,0,-1):
            comparisons+=1; left=idx*2
            if levels_a[level-1][left]!=levels_b[level-1][left]: idx=left
            else: idx=left+1
        rows.append({"blocks":blocks,"comparisons":comparisons,"identified_block":idx,"expected":blocks//2})
    return rows

def collisions_cycles():
    rows=[]
    for name in ("C","E","H","H2"):
        seen={}; collision=None
        for value in range(1,1<<12):
            seq=[(value>>i)&1 for i in range(12)]; s=enc(seq,name)
            if s in seen: collision={"first":seen[s],"second":seq}; break
            seen[s]=seq
        cycle=None; seen_cycle={}; current=enc([],name); zero=enc([0],name)
        for i in range(100000):
            if current in seen_cycle: cycle={"start":seen_cycle[current],"length":i-seen_cycle[current]}; break
            seen_cycle[current]=i
            if name=="H": current=affine_combine(current,zero)
            elif name=="H2": current=h2_combine(current,zero)
            else:
                words=list(current); update_words(words,0,2 if name=="C" else 4); current=tuple(words)
        rows.append({"algorithm":name,"binary_collision":collision,"zero_symbol_cycle":cycle})
    return rows

def main():
    started=time.perf_counter(); result={"versions":{"C":"C-1.0","E":"E-1.0","H":"H-1.0","H2":"H2-1.0"},"hard_trace":hard_trace(),"hard_trace_nearest":hard_trace_nearest(),"C_rollback":c_rollback(),"algebra":algebra(),"windows":windows(),"hierarchy":hierarchy(),"collisions_cycles":collisions_cycles()}
    result["elapsed_seconds"]=round(time.perf_counter()-started,3); root=Path(__file__).resolve().parents[1]/"research"; (root/"survivor_consolidation.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
    print(json.dumps({"elapsed_seconds":result["elapsed_seconds"],"rollback":result["C_rollback"],"hard_trace":result["hard_trace"]}))
if __name__=="__main__": main()
