"""Explicitly permissioned, local, document-disjoint text training data.

Manifest documents MUST name their own source, licensing/permission basis,
and preassigned train/validation split.  Never scans arbitrary user folders.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import hashlib, json
import torch
from .model import BOS, EOS, PAD

@dataclass(frozen=True)
class Document:
    filename: str
    content: bytes
    split: str
    source: str
    license: str
    sha256: str

class Corpus:
    def __init__(self, manifest: str, shuffle_seed: int | None = None):
        source = Path(manifest).resolve()
        data = json.loads(source.read_text(encoding='utf-8'))
        if data.get('format') != 'genesis.corpus.v1':
            raise ValueError('Unsupported corpus manifest')
        documents = data.get('documents')
        if not isinstance(documents,list) or len(documents)<2:
            raise ValueError('Supply at least one train and one validation document')
        self.documents=[]
        seen={}
        hashes=[]
        # Deterministic document ordering for reproducibility.
        ordered_entries = sorted(documents, key=lambda e: e.get('file',''))
        for entry in ordered_entries:
            if not isinstance(entry,dict) or entry.get('training_permitted') is not True:
                raise ValueError('Explicit training permission required for each document')
            filename=entry.get('file')
            split=entry.get('split')
            license_name=entry.get('license')
            origin=entry.get('source')
            if not isinstance(filename,str) or not filename.endswith('.txt') or not filename:
                raise ValueError('Only explicit UTF-8 .txt documents are permitted')
            if split not in ('train','validation','test') or not isinstance(license_name,str) or not license_name.strip() or not isinstance(origin,str) or not origin.strip():
                raise ValueError('Document requires split, license and source')
            path=(source.parent/filename).resolve()
            if not path.is_relative_to(source.parent) or not path.is_file():
                raise ValueError('Document missing or escapes corpus directory')
            if path.stat().st_size > 8*1024*1024:
                raise ValueError('Max 8 MiB per text document for prototype')
            contents=path.read_bytes()
            contents.decode('utf-8',errors='strict')
            if not contents.strip(): raise ValueError('Empty document')
            digest=hashlib.sha256(contents).hexdigest()
            if digest in seen:
                raise ValueError(f'Duplicate document bytes, possible split leakage: {filename} and {seen[digest]}')
            seen[digest]=filename
            self.documents.append(Document(filename,contents,split,origin,license_name,digest))
            hashes.append({'file':filename,'content_sha256':digest,'split':split,
                           'source':origin,'license':license_name})
        self.train=[x for x in self.documents if x.split=='train']
        self.validation=[x for x in self.documents if x.split=='validation']
        self.test=[x for x in self.documents if x.split=='test']
        if len(self.train)<1 or len(self.validation)<1:
            raise ValueError('Both train and validation splits are mandatory')
        self.provenance=hashes
        canonical=json.dumps(hashes,sort_keys=True,ensure_ascii=True).encode('utf-8')
        self.fingerprint=hashlib.sha256(canonical).hexdigest()

    @staticmethod
    def _window(document: Document, context: int, start: int) -> tuple[list[int], list[int]]:
        # Consistent next-byte language-model objective. Keep BOS/EOS boundaries.
        all_tokens=[BOS]+list(document.content)+[EOS]
        seq=all_tokens[start:start+context+1]
        return seq[:-1],seq[1:]

    def sample(self, batch:int, context:int, device:str, seed: int | None = None) -> tuple[torch.Tensor,torch.Tensor]:
        x=torch.full((batch,context),PAD,dtype=torch.long)
        y=torch.full((batch,context),-100,dtype=torch.long)
        gen = torch.Generator(device='cpu') if seed is not None else None
        if seed is not None:
            gen.manual_seed(seed)
        for row in range(batch):
            doc_idx = int(torch.randint(len(self.train),(1,),generator=gen).item())
            doc = self.train[doc_idx]
            total=len(doc.content)+2
            if torch.rand((), generator=gen).item() < 0.20:
                start=0
            else:
                start=int(torch.randint(max(1,total-context+1),(1,),generator=gen).item())
            a,b=self._window(doc,context,start)
            x[row,:len(a)]=torch.tensor(a)
            y[row,:len(b)]=torch.tensor(b)
        return x.to(device),y.to(device)

    def validation_windows(self,context:int,device:str,max_windows:int=32):
        windows=0
        for doc in self.validation:
            length=len(doc.content)+2
            for start in range(0,length-1,context):
                if windows>=max_windows:return
                a,b=self._window(doc,context,start)
                if not a:continue
                x=torch.full((1,context),PAD,dtype=torch.long,device=device)
                y=torch.full((1,context),-100,dtype=torch.long,device=device)
                x[0,:len(a)]=torch.tensor(a,device=device)
                y[0,:len(b)]=torch.tensor(b,device=device)
                windows+=1
                yield x,y
