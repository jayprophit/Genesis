"""Actual original model pretraining on provided consented UTF-8 documents.

A native-original text model trainer: no downloaded tokenizer or model weights.
Splits, checkpoint resume, corpus integrity check, held-out loss and provenance.
"""
from __future__ import annotations
import argparse, json, math
from dataclasses import asdict
from pathlib import Path
import torch
import torch.nn.functional as F
from .model import GenesisSeed, GenesisConfig, SeedConfig, VOCAB_SIZE
from .corpus import Corpus

@torch.no_grad()
def validation(model, corpus:Corpus, device:str, max_windows:int=32)->dict:
    model.eval()
    total_loss=0.;count=0;windows=0
    cfg = model.seed_config
    for x,y in corpus.validation_windows(cfg.context,device,max_windows):
        logits=model(x)
        total_loss+=float(F.cross_entropy(logits.reshape(-1,VOCAB_SIZE),y.reshape(-1),
                                     ignore_index=-100,reduction='sum').item())
        count+=int((y!=-100).sum().item());windows+=1
    ce=total_loss/count
    return {'validation_cross_entropy':round(ce,5),'validation_perplexity':round(math.exp(min(ce,40)),3),
            'validation_tokens':count,'validation_windows':windows,'heldout_by':'document'}

def detect_overfitting(train_loss: float, val_loss: float, threshold: float = 0.15) -> str:
    """Return a simple overfitting status for a training run.

    Overfitting is flagged when the validation loss is substantially higher than
    the training loss, signalling that memorization is outpacing generalization.
    """
    if val_loss <= 0 or train_loss <= 0:
        return 'insufficient_loss'
    ratio = val_loss / train_loss
    if ratio > 1 + threshold:
        return 'overfitting'
    if ratio > 1.0:
        return 'underfitting'
    return 'good_fit'

def train(manifest:str,output:str,steps:int=40,batch_size:int=4,width:int=64,
          context:int=128,seed:int=42,device:str='cpu',resume:str|None=None,
          lr:float=0.001,max_val_windows:int=32,layers:int=2)->dict:
    if steps<1 or batch_size<1 or lr<=0:raise ValueError('steps, batch, lr must be positive')
    corpus=Corpus(manifest)
    torch.set_num_threads(min(4,torch.get_num_threads()))
    if resume:
        saved=torch.load(resume,map_location='cpu',weights_only=True)
        if saved.get('trainer')!='genesis_corpus_v0.3' or saved.get('corpus_fingerprint')!=corpus.fingerprint:
            raise ValueError('Refusing resume: not an original Genesis corpus checkpoint or changed corpus')
        cfg=GenesisConfig.from_dict(saved['config'])
        model=GenesisSeed(cfg)
        model.load_state_dict(saved['state_dict'],strict=True)
        opt=torch.optim.AdamW(model.parameters(),lr=lr)
        opt.load_state_dict(saved['optimizer'])
        torch.set_rng_state(saved['torch_rng_state'])
        start=int(saved['step'])
        history=list(saved['history'])
        if device=='cuda' and 'cuda_rng_state' in saved:
            torch.cuda.set_rng_state_all(saved['cuda_rng_state'])
    else:
        torch.manual_seed(seed)
        if device=='cuda':torch.cuda.manual_seed_all(seed)
        cfg=GenesisConfig(seed_config=SeedConfig(width=width,context=context,layers=layers))
        model=GenesisSeed(cfg)
        opt=torch.optim.AdamW(model.parameters(),lr=lr)
        start=0;history=[]
    model.to(device)
    # AdamW moves optimizer buffers to their corresponding device on checkpoint resume.
    for values in opt.state.values():
        for k,value in values.items():
            if isinstance(value,torch.Tensor):values[k]=value.to(device)
    baseline=validation(model,corpus,device,max_val_windows)
    for step in range(start,start+steps):
        model.train()
        x,y=corpus.sample(batch_size,model.seed_config.context,device)
        logits=model(x)
        loss=F.cross_entropy(logits.reshape(-1,VOCAB_SIZE),y.reshape(-1),ignore_index=-100)
        opt.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(),1.)
        opt.step()
        if step==start or (step+1)%max(1,steps//4)==0 or step==start+steps-1:
            history.append({'step':step+1,'train_cross_entropy':round(float(loss.detach().item()),5)})
    result=validation(model,corpus,device,max_val_windows)
    # Detect overfitting: compare held-out validation loss against last training loss.
    last_train = history[-1]['train_cross_entropy'] if history else result['validation_cross_entropy']
    overfit_status = detect_overfitting(last_train, result['validation_cross_entropy'])
    out=Path(output);out.parent.mkdir(parents=True,exist_ok=True)
    obj={'trainer':'genesis_corpus_v0.3','config':model.state_config(),
         'state_dict':{k:v.detach().cpu() for k,v in model.state_dict().items()},
         'optimizer':opt.state_dict(),'torch_rng_state':torch.get_rng_state(),
         'step':start+steps,'history':history,'corpus_fingerprint':corpus.fingerprint,
         'source_documents':corpus.provenance,'seed':seed,'initial_validation':baseline,
         'current_validation':result,'deterministic':{'shuffle_seed':seed,'batch_seed':seed},
         'overfit_status':overfit_status}
    if device=='cuda':obj['cuda_rng_state']=torch.cuda.get_rng_state_all()
    # Save atomically so interrupted writes don't destroy the last checkpoint.
    temp=out.with_name(out.name+'.tmp')
    torch.save(obj,temp);temp.replace(out)
    response={'checkpoint':str(out),'step':start+steps,
              'new_steps':steps,'corpus_fingerprint':corpus.fingerprint,
              'train_documents':len(corpus.train),'validation_documents':len(corpus.validation),
              'beginning_of_this_run':baseline,'after_this_run':result,
              'history_tail':history[-5:]}
    print(json.dumps(response,indent=2))
    return response

def main():
    p=argparse.ArgumentParser(description='Train Genesis own model from random weights or resume its own checkpoint')
    p.add_argument('--manifest',required=True)
    p.add_argument('--output',required=True)
    p.add_argument('--resume')
    p.add_argument('--steps',type=int,default=40)
    p.add_argument('--batch-size',type=int,default=4)
    p.add_argument('--width',type=int,choices=[32,64,128,256],default=64)
    p.add_argument('--layers',type=int,default=2)
    p.add_argument('--context',type=int,default=128)
    p.add_argument('--seed',type=int,default=42)
    p.add_argument('--lr',type=float,default=0.001)
    p.add_argument('--device',choices=['cpu','cuda'],default='cpu')
    p.add_argument('--max-val-windows',type=int,default=32)
    a=p.parse_args()
    train(a.manifest,a.output,a.steps,a.batch_size,a.width,a.context,a.seed,a.device,a.resume,a.lr,a.max_val_windows,a.layers)
if __name__=='__main__':main()
