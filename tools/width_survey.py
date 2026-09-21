#!/usr/bin/env python3
import json, random
from pathlib import Path
from research_batch import encode, hamming

def main():
    rng=random.Random(20260921); rows=[]
    for width in (64,128,256,512,1024,2048,4096):
        for kind in range(6):
            states=[]; distances=[]
            for _ in range(200):
                seq=[rng.randrange(256) for _ in range(16)]
                mutated=seq[:]; mutated[rng.randrange(16)]^=1
                states.append(encode(seq,width,f"cand{kind}")); distances.append(hamming(states[-1],encode(mutated,width,f"cand{kind}"),width))
            rows.append({"width":width,"algorithm":f"cand{kind}","unique_of_200":len(set(states)),"mean_one_mutation_hamming":sum(distances)/len(distances)})
    out=Path(__file__).resolve().parents[1]/"research"/"width_survey.json"; out.write_text(json.dumps(rows,indent=2),encoding="utf-8")
    print(json.dumps({"rows":len(rows),"widths":[64,128,256,512,1024,2048,4096]}))
if __name__=="__main__": main()
