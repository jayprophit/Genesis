from __future__ import annotations
import argparse
import torch
from .train import load_checkpoint
from .data import synthetic_image
from .camera import capture_frame,frame_to_image
from .speech import write_wave


def main():
    p=argparse.ArgumentParser(description='First-party Genesis Seed demo')
    p.add_argument('--checkpoint',default='genesis_seed.pt')
    sub=p.add_subparsers(dest='command',required=True)
    t=sub.add_parser('text'); t.add_argument('prompt');t.add_argument('--max-new-tokens',type=int,default=48)
    v=sub.add_parser('vision');v.add_argument('--color',choices=['red','green','blue'],default='red');v.add_argument('--shape',choices=['square','circle'],default='square')
    c=sub.add_parser('camera');c.add_argument('--enable-camera',action='store_true');c.add_argument('--index',type=int,default=0)
    s=sub.add_parser('sound');s.add_argument('text');s.add_argument('--out',default='genesis_toy_voice.wav')
    args=p.parse_args()
    if args.command=='sound':
        write_wave(args.text,args.out)
        print('Experimental waveform written:',args.out,'(NOT intelligible natural TTS)')
        return
    if args.command=='camera' and not args.enable_camera:
        raise PermissionError('Explicit --enable-camera required; camera not opened')
    model=load_checkpoint(args.checkpoint)
    if args.command=='text':
        print('Toy-model continuation:',model.generate(prompt=args.prompt,max_new_tokens=args.max_new_tokens))
    elif args.command=='vision':
        image=synthetic_image(args.color,args.shape,seed=999)[None]
        print('Toy visual caption:',model.generate(image=image,max_new_tokens=32))
    elif args.command=='camera':
        frame=capture_frame(args.index,user_authorized=args.enable_camera)
        image=frame_to_image(frame,model.config.image_size)
        print('Out-of-distribution experimental camera caption:',model.generate(image=image,max_new_tokens=32))
        print('WARNING: only toy synthetic-shape images were trained. Do not trust real-camera descriptions.')
if __name__=='__main__':main()
