"""Command-line client for Genesis's original independent NumPy CPU runtime."""
from __future__ import annotations
import argparse
from .native_runtime import GenesisNative
from .server_native import COLORS,SHAPES,toy_image

def main():
    p=argparse.ArgumentParser(description='Run original Genesis model using native NumPy inference; no PyTorch required')
    p.add_argument('--model',default='models/native_visual_seed')
    sub=p.add_subparsers(dest='command',required=True)
    t=sub.add_parser('text');t.add_argument('prompt');t.add_argument('--max-new-tokens',type=int,default=48)
    v=sub.add_parser('vision');v.add_argument('--color',choices=list(COLORS),default='red');v.add_argument('--shape',choices=SHAPES,default='square');v.add_argument('--seed',type=int,default=923)
    a=p.parse_args()
    engine=GenesisNative(a.model)
    if a.command=='text':
        print(engine.generate(prompt=a.prompt,max_new_tokens=a.max_new_tokens))
    else:
        print(engine.generate(image=toy_image(a.color,a.shape,a.seed),max_new_tokens=32))
if __name__=='__main__':main()
