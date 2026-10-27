"""Independent holdout *within the same synthetic image generator family*.
No claim of generalization to actual photos, people or live camera vision.
"""
from __future__ import annotations
import argparse,json
from pathlib import Path
from .train import load_checkpoint
from .data import COLORS,SHAPES,synthetic_image


def evaluate(path:str='genesis_seed.pt',first_seed:int=900,n_seeds:int=10,output:str|None=None)->dict:
    if n_seeds<1 or first_seed<0:raise ValueError('bad evaluation range')
    model=load_checkpoint(path)
    table=[]
    for color in COLORS:
        for shape in SHAPES:
            target=f'{color} {shape}'
            predictions=[model.generate(image=synthetic_image(color,shape,seed=z)[None],max_new_tokens=32)
                         for z in range(first_seed,first_seed+n_seeds)]
            table.append({'target':target,'n':n_seeds,'correct':sum(x==target for x in predictions),
                          'example':predictions[0]})
    total=sum(x['n'] for x in table)
    correct=sum(x['correct'] for x in table)
    result={'evaluation':'synthetic_image_family_holdout','seed_range':[first_seed,first_seed+n_seeds-1],
            'total':total,'correct':correct,'accuracy':correct/total,'per_class':table,
            'interpretation':'Not a real camera benchmark; not evidence of general vision.'}
    if output:Path(output).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
    return result


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--checkpoint',default='genesis_seed.pt')
    p.add_argument('--first-seed',type=int,default=900)
    p.add_argument('--n-seeds',type=int,default=10)
    p.add_argument('--output')
    a=p.parse_args();evaluate(a.checkpoint,a.first_seed,a.n_seeds,a.output)
if __name__=='__main__':main()
