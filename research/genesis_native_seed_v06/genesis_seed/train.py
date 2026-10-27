from __future__ import annotations
import argparse, json, random
from pathlib import Path
import torch
import torch.nn.functional as F
from .model import GenesisSeed, GenesisConfig, SeedConfig
from .data import TRAIN_TEXT, synthetic_batch, synthetic_image, next_token_batch


def train(steps:int=160, output:str='genesis_seed.pt', device:str='cpu', seed:int=42,
          log_every:int=25, width:int=64) -> dict:
    if steps <= 0:
        raise ValueError('steps must be positive')
    torch.manual_seed(seed)
    random.seed(seed)
    if device == 'cpu':
        torch.set_num_threads(min(4,torch.get_num_threads()))
    model=GenesisSeed(SeedConfig(width=width)).to(device)
    opt=torch.optim.AdamW(model.parameters(),lr=0.0015,weight_decay=0.01)
    history=[]
    model.train()
    for step in range(steps):
        # text next-byte prediction: same trainable backbone as image-text prediction
        texts=random.choices(TRAIN_TEXT,k=4)
        tx,ty=next_token_batch(texts,device)
        ix,iy,im=synthetic_batch(8,device,seed=seed+step)
        tl=F.cross_entropy(model(tx).flatten(0,1),ty.flatten(),ignore_index=-100)
        il=F.cross_entropy(model(ix,im).flatten(0,1),iy.flatten(),ignore_index=-100)
        loss=tl+il
        opt.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(),1.)
        opt.step()
        if step==0 or (step+1)%log_every==0 or step==steps-1:
            record={'step':step+1,'text_ce':round(float(tl.item()),4),
                    'vision_caption_ce':round(float(il.item()),4)}
            history.append(record)
            print(json.dumps(record),flush=True)
    path=Path(output)
    path.parent.mkdir(parents=True,exist_ok=True)
    torch.save({'config':model.state_config(),'state_dict':model.state_dict(),
                'seed':seed,'steps':steps,'training_history':history,
                'training_data':'bundled toy text and synthetic shapes'},path)
    model.eval()
    examples={}
    for color,shape in [('red','square'),('blue','circle'),('green','square')]:
        pic=synthetic_image(color,shape,seed=2203)[None].to(device)
        examples[f'{color} {shape}']=model.generate(image=pic,max_new_tokens=24)
    print(json.dumps({'checkpoint':str(path),'size_bytes':path.stat().st_size,
                       'visual_examples':examples},indent=2))
    return {'checkpoint':str(path),'history':history,'examples':examples}


def load_checkpoint(path:str,device:str='cpu') -> GenesisSeed:
    # weights_only=True: never unpickle arbitrary code from model checkpoints.
    saved=torch.load(path,map_location=device,weights_only=True)
    cfg=GenesisConfig.from_dict(saved['config'])
    model=GenesisSeed(cfg).to(device)
    model.load_state_dict(saved['state_dict'],strict=True)
    model.eval()
    return model


def main():
    p=argparse.ArgumentParser(description='Train Genesis Seed from random initialization; no pretrained model')
    p.add_argument('--steps',type=int,default=160)
    p.add_argument('--output',default='genesis_seed.pt')
    p.add_argument('--device',choices=['cpu','cuda'],default='cpu')
    p.add_argument('--seed',type=int,default=42)
    p.add_argument('--width',type=int,choices=[32,64,128],default=64)
    args=p.parse_args()
    train(steps=args.steps,output=args.output,device=args.device,seed=args.seed,width=args.width)

if __name__=='__main__':main()
