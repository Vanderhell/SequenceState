#!/usr/bin/env python3
"""Reproducible offline SequenceState research batch.

This is deliberately an experiment harness, not production core code. It uses
fixed seeds and small linear probes to measure what a frozen state exposes.
"""
from __future__ import annotations
import json, math, os, random, statistics, time
from collections import Counter
from pathlib import Path
import numpy as np

MASK = (1 << 32) - 1
SEEDS = (7, 19, 101)
WIDTHS = (64, 256, 1024)
LENGTHS = (1, 2, 4, 8, 16, 32, 64, 128, 256, 1024, 4096, 16384, 65536, 262144)

def rot(x, n):
    n &= 31
    return ((x << n) | (x >> ((32 - n) & 31))) & MASK

def mix(x):
    x &= MASK; x ^= x >> 16; x = x * 0x7FEB352D & MASK
    x ^= x >> 15; x = x * 0x846CA68B & MASK
    return (x ^ (x >> 16)) & MASK

def init_words(width):
    return [mix(0x243F6A88 + i * 0x9E3779B9) for i in range(width // 32)]

def update_words(words, x, kind):
    x &= MASK; k = mix(x + 0x9E3779B9 * (kind + 1))
    n = len(words)
    if kind == 0:
        for i in range(n): words[i] = (rot(words[i] + k + i * 0x632BE59B, x + i * 3) ^ words[(i + 3) % n]) & MASK
    elif kind == 1:
        for i in range(n): words[i] = ((words[i] * ((k | 1) + i * 2) + rot(words[(i + 1) % n], 11)) ^ k) & MASK
    elif kind == 2:
        for i in range(n): words[i] = rot(words[i] ^ k, i * 7 + x) * 0x9E3779B1 & MASK
        words[0], words[min(3, n - 1)], words[min(6, n - 1)] = words[min(3, n - 1)], words[min(6, n - 1)], words[0]
    elif kind == 3:
        for i in range(n): words[i] ^= rot(k + i * 0x45D9F3B, i * 4)
    elif kind == 4:
        p = (x ^ k) % n
        for i in range(n): words[i] = rot(words[(i + p) % n] + k, i + p)
    else:
        for i in range(n): words[i] = (words[i] + rot(k ^ words[(i + 1) % n], i * 3)) & MASK

def state(seq, width, kind):
    words = init_words(width)
    for x in seq: update_words(words, x, kind)
    return tuple(words)

def affine_state(seq, width=512):
    """H: accumulated per-lane affine transformations modulo 2^32."""
    n = width // 64
    aa = [1] * n; bb = [0] * n
    for x in seq:
        for i in range(n):
            a = (2 * mix(x + i * 0x9E3779B9) + 1) & MASK
            b = mix(x ^ (i * 0xA5A5A5A5))
            bb[i] = (a * bb[i] + b) & MASK
            aa[i] = (a * aa[i]) & MASK
    return tuple(aa + bb)

def affine_combine(left, right):
    n = len(left) // 2; la, lb = left[:n], left[n:]; ra, rb = right[:n], right[n:]
    return tuple(((ra[i] * la[i]) & MASK) for i in range(n)) + tuple(((ra[i] * lb[i] + rb[i]) & MASK) for i in range(n))

def baseline(seq, width, name):
    bits = width
    if name == "xor": return (sum((x & MASK) << (32 * i) for i, x in enumerate(seq[:max(1, width // 32)])) & ((1 << width) - 1),)
    if name == "sum": return (sum(seq) & ((1 << width) - 1),)
    if name in ("fnv", "poly", "crc"):
        v = 2166136261 if name == "fnv" else (0 if name == "crc" else 1)
        for x in seq:
            if name == "fnv": v = ((v ^ x) * 16777619) & MASK
            elif name == "poly": v = (v * 65599 + x + 1) & MASK
            else:
                v ^= x
                for _ in range(32): v = ((v >> 1) ^ 0xEDB88320) if v & 1 else v >> 1
        return (v,)
    # Bloom, position-aware Bloom and Count-Min Sketch share a fixed bit/word budget.
    out = [0] * max(1, width // 32)
    for pos, x in enumerate(seq):
        for h in range(4):
            z = mix(x + h * 0x9E3779B9 + (pos * 0xD1B54A32 if name == "pos_bloom" else 0)) % width
            if name == "cms": out[(h * width // 4 + z) // 32 % len(out)] = (out[(h * width // 4 + z) // 32 % len(out)] + 1) & MASK
            else: out[z // 32] |= 1 << (z % 32)
    return tuple(out)

def vsa(seq, width):
    """Deterministic binary bipolar VSA with position rotation and superposition."""
    n = width; vec = np.zeros(n, dtype=np.int16)
    for pos, x in enumerate(seq):
        rng = np.random.default_rng((x * 1000003 + pos * 9176 + 17) & 0xffffffff)
        proj = rng.integers(0, 2, n, dtype=np.int8) * 2 - 1
        shift = pos % n; vec += np.roll(proj, shift)
    return tuple((vec >= 0).astype(np.uint8).tolist())

def bits(st, width):
    if isinstance(st[0], int) and width <= 4096:
        return np.asarray([(word >> j) & 1 for word in st for j in range(32)], dtype=np.float64)[:width]
    return np.asarray(st, dtype=np.float64)

def encode(seq, width, name):
    if name.startswith("cand"):
        return state(seq, width, int(name[-1]))
    if name == "H": return affine_state(seq, max(64, width))
    if name == "vsa": return vsa(seq, width)
    return baseline(seq, width, name)

def hamming(a, b, width):
    if a and isinstance(a[0], int): return sum((x ^ y).bit_count() for x, y in zip(a, b))
    return int(np.count_nonzero(np.asarray(a) != np.asarray(b)))

def exhaustive(width, kind, max_len=10):
    result = []
    for n in range(1, max_len + 1):
        seen = Counter()
        for value in range(1 << n):
            seq = [(value >> i) & 1 for i in range(n)]
            seen[encode(seq, width, f"cand{kind}")] += 1
        collisions = sum(c - 1 for c in seen.values() if c > 1)
        result.append({"length": n, "sequences": 1 << n, "unique": len(seen), "excess_collisions": collisions})
    return result

def ridge_probe(X, y, seed=1):
    rng = np.random.default_rng(seed); idx = rng.permutation(len(y)); cut = max(1, int(len(y) * .7))
    tr, te = idx[:cut], idx[cut:]; xtr = np.column_stack((np.ones(len(tr)), X[tr])); xte = np.column_stack((np.ones(len(te)), X[te]))
    reg = 1e-2 * np.eye(xtr.shape[1]); reg[0, 0] = 0
    w = np.linalg.solve(xtr.T @ xtr + reg, xtr.T @ y[tr]); pred = (xte @ w >= 0.5).astype(int)
    return float(np.mean(pred == y[te]))

def task_dataset(task, n, alphabet, seed, width, name):
    rng = random.Random(seed); X=[]; y=[]
    for _ in range(n):
        label = rng.randrange(2)
        if task == "membership":
            # Shorter streams and explicit construction prevent all-positive labels.
            seq = [rng.randrange(alphabet) for _ in range(12)]
            q = rng.randrange(alphabet)
            if label: seq[0] = q
            else: seq = [x if x != q else (x + 1) % alphabet for x in seq]
        elif task == "pair_order":
            seq = [rng.randrange(alphabet) for _ in range(24)]
            a, b = 0, 1
            seq[0], seq[-1] = (a, b) if label else (b, a)
        elif task == "pattern":
            pat = [0, 1, 2][:min(3, alphabet)]
            seq = [rng.randrange(alphabet) for _ in range(16)]
            if label: seq[:len(pat)] = pat
            else:
                while any(seq[i:i+len(pat)] == pat for i in range(len(seq))):
                    seq = [rng.randrange(alphabet) for _ in range(16)]
        else: raise ValueError(task)
        X.append(bits(encode(seq, width, name), width)); y.append(label)
    return np.asarray(X), np.asarray(y)

def task_matrix():
    rows=[]
    names=[f"cand{i}" for i in range(6)] + ["H", "bloom", "pos_bloom", "cms", "vsa", "fnv"]
    for width in (64, 256):
        for task in ("membership", "pair_order", "pattern"):
            for name in names:
                actual = width if name != "H" else 512
                X,y=task_dataset(task, 240, 4, 3000+width, actual, name)
                rows.append({"width":actual,"task":task,"algorithm":name,"accuracy":ridge_probe(X,y,actual+len(task))})
    return rows

def adversarial():
    rows=[]
    seqs={"zeros":[0]*64,"ones":[1]*64,"alternating":[i&1 for i in range(64)],"period4":[i&3 for i in range(64)],"increment":list(range(64)),"reverse":list(range(63,-1,-1))}
    for name, seq in seqs.items():
        for kind in range(6):
            a=encode(seq,256,f"cand{kind}"); b=encode(seq[::-1],256,f"cand{kind}")
            rows.append({"sequence":name,"algorithm":f"cand{kind}","reverse_distance":hamming(a,b,256)})
    return rows

def similarity_and_collisions():
    similarity=[]; collisions=[]; rng=random.Random(424242)
    names=[f"cand{i}" for i in range(6)]+["H","bloom","pos_bloom","vsa","fnv"]
    for name in names:
        distances=[]; edits=[]
        for _ in range(160):
            base=[rng.randrange(16) for _ in range(32)]; changed=base[:]
            count = rng.randrange(1, 17)
            for i in rng.sample(range(32), count): changed[i]=rng.randrange(16)
            distances.append(hamming(encode(base,256 if name!="H" else 512,name),encode(changed,256 if name!="H" else 512,name),256 if name!="H" else 512))
            edits.append(count)
        correlation=float(np.corrcoef(distances, edits)[0,1]) if np.std(distances)>0 and np.std(edits)>0 else 0.0
        similarity.append({"algorithm":name,"one_replacement_distance_mean":float(np.mean(distances)),"similarity_correlation_degenerate":correlation})
        seen=set(); exact=0
        for _ in range(10000):
            seq=tuple(rng.randrange(4) for _ in range(16)); value=encode(seq,64 if name not in ("H","vsa") else (512 if name=="H" else 64),name)
            if value in seen: exact += 1
            seen.add(value)
        collisions.append({"algorithm":name,"samples":10000,"exact_collisions":exact,"unique":len(seen)})
    return similarity, collisions

def main():
    started=time.perf_counter(); result={"version":"research-batch-1","seeds":SEEDS,"widths":WIDTHS,"lengths":LENGTHS}
    result["exhaustive"]={f"cand{k}":exhaustive(64,k) for k in range(6)}
    result["tasks"]=task_matrix(); result["adversarial"]=adversarial()
    result["similarity"], result["collisions"] = similarity_and_collisions()
    # Long-range marker degradation, using a frozen linear probe on marker identity.
    long=[]
    for name in ("cand0","cand1","cand2","cand3","cand4","cand5","H","vsa"):
        for length in (16,64,256,1024,4096):
            X=[]; y=[]
            for seed in range(80):
                rng=random.Random(seed); marker=seed & 3; seq=[marker]+[rng.randrange(4) for _ in range(length-1)]
                X.append(bits(encode(seq,256 if name!="H" else 512,name),256 if name!="H" else 512)); y.append(marker & 1)
            long.append({"algorithm":name,"length":length,"marker_bit_accuracy":ridge_probe(np.asarray(X),np.asarray(y),length)})
    result["long_range"]=long
    # Exact composition proof for H and explicit non-composability observation for A-F.
    comp=[]
    for seed in range(1000):
        rng=random.Random(seed); seq=[rng.randrange(256) for _ in range(24)]; cut=seed%25
        left,right=seq[:cut],seq[cut:]
        comp.append({"candidate":"H","exact":affine_state(seq)==affine_combine(affine_state(left),affine_state(right))})
    result["composition"]={"H_exact_fraction":sum(x["exact"] for x in comp)/len(comp),"A_to_F_combine": "not available from vector state"}
    # Smallest regression witness for D's XOR-like degeneracy.
    result["regressions"]={"candidate_D_binary_length_10_unique_states": result["exhaustive"]["cand3"][-1]["unique"],"cause":"symbol-dependent lane XOR has no state feedback"}
    if result["regressions"]["candidate_D_binary_length_10_unique_states"] != 2:
        raise AssertionError("candidate D regression changed: expected exactly two binary length-10 states")
    result["elapsed_seconds"]=round(time.perf_counter()-started,3)
    root=Path(__file__).resolve().parents[1]/"research"; root.mkdir(exist_ok=True)
    (root/"research_results.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
    with (root/"research_summary.csv").open("w",encoding="utf-8") as f:
        f.write("algorithm,task,width,accuracy\n")
        for r in result["tasks"]: f.write(f'{r["algorithm"]},{r["task"]},{r["width"]},{r["accuracy"]:.6f}\n')
    print(json.dumps({"elapsed_seconds":result["elapsed_seconds"],"composition":result["composition"],"task_rows":len(result["tasks"]),"long_rows":len(long)}))

if __name__ == "__main__": main()
