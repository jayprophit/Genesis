"""First-party NumPy CPU inference for our own GenesisSeed architecture.

NO torch, Ollama, internet access, Hugging Face or model serving dependency.
Original kernels for projection, causal multihead attention, GELU, layer norm,
RGB patch encoding and autoregressive decoding.  Research CPU runtime; not
an optimized C++/CUDA implementation or replacement for a full model server.
"""
from __future__ import annotations
import hashlib
import json
import math
from pathlib import Path
import numpy as np

BOS, EOS, IMG, PAD = 256, 257, 258, 259


def encode(text: str) -> list[int]:
    return list(text.encode('utf-8'))


def decode(ids: list[int]) -> str:
    return bytes(x for x in ids if 0 <= x < 256).decode('utf-8', errors='replace')


def layernorm(x: np.ndarray, w: np.ndarray, b: np.ndarray) -> np.ndarray:
    # Matches torch.nn.LayerNorm (biased population variance, eps=1e-5).
    mu = x.mean(axis=-1, keepdims=True)
    var = ((x-mu)**2).mean(axis=-1, keepdims=True)
    return (x-mu)/np.sqrt(var+1e-5)*w+b


def gelu(x: np.ndarray) -> np.ndarray:
    # Reference, exact erf GELU used by torch.nn.TransformerEncoderLayer.
    # Scalar math.erf is deliberately used for portability, at a speed cost.
    erf = np.vectorize(math.erf, otypes=[np.float32])
    return (0.5*x*(1.0+erf(x/np.float32(math.sqrt(2.0))))).astype(np.float32)


class GenesisNative:
    """Inference-only copy of GenesisSeed with our own NumPy tensor operations."""
    def __init__(self, folder: str | Path):
        folder = Path(folder)
        man = json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
        if man.get('format') != 'genesis.numpy.inference.v1' or man.get('architecture') != 'GenesisSeed':
            raise ValueError('Unsupported original Genesis checkpoint architecture')
        if man.get('weights_file') != 'weights.npz':
            raise ValueError('Invalid model archive path')
        config = man['config']
        expected = {'width','heads','layers','context','image_size','patch_size','dropout'}
        if set(config) != expected or not (1 <= config['layers'] <= 16 and
                1 <= config['width'] <= 2048 and config['width'] % config['heads'] == 0 and
                1 <= config['context'] <= 8192 and
                config['image_size'] % config['patch_size'] == 0):
            raise ValueError('Malformed model configuration')
        archive = folder/'weights.npz'
        digest=hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != man['weights_sha256']:
            raise ValueError('Model checksum verification failed')
        with np.load(archive, allow_pickle=False) as f:
            self.w = {key: f[key].astype(np.float32, copy=False) for key in f.files}
        if len(self.w) != man['tensor_count'] or sum(x.size for x in self.w.values()) != man['parameter_count']:
            raise ValueError('Weight inventory does not match manifest')
        self.config = config
        self.manifest=man
        width = config['width']
        self.vocab_size = self.manifest.get('vocab_size', 260)
        assert self.w['token_embedding.weight'].shape == (self.vocab_size,width)
        assert self.w['position.weight'].shape == (config['context'],width)

    def weight(self, key: str) -> np.ndarray:
        return self.w[key]

    def linear(self, x: np.ndarray, prefix: str) -> np.ndarray:
        return x @ self.weight(prefix+'.weight').T + self.weight(prefix+'.bias')

    def rgb_patches(self, image: np.ndarray) -> np.ndarray:
        n, p = self.config['image_size'], self.config['patch_size']
        if image.shape not in ((3,n,n),(1,3,n,n)):
            raise ValueError(f'image must be RGB [3,{n},{n}] or [1,3,{n},{n}]')
        if image.ndim == 4:
            image=image[0]
        image=np.asarray(image,dtype=np.float32)
        if not np.isfinite(image).all():
            raise ValueError('Image has NaN/inf')
        patches=np.stack([image[:,i:i+p,j:j+p].reshape(-1)
                    for i in range(0,n,p) for j in range(0,n,p)])
        conv=self.weight('patch_embedding.weight').reshape(self.config['width'],-1)
        return patches @ conv.T + self.weight('patch_embedding.bias')

    def forward(self, ids: list[int], image: np.ndarray | None = None) -> np.ndarray:
        if len(ids)<1 or any(not isinstance(x,int) or x<0 or x>=self.vocab_size for x in ids):
            raise ValueError('One or more invalid byte/control tokens')
        stream = []
        prefix=0
        if image is not None:
            stream.append(self.weight('token_embedding.weight')[[IMG]])
            stream.append(self.rgb_patches(image))
            prefix=1+stream[1].shape[0]
        stream.append(self.weight('token_embedding.weight')[ids])
        x=np.concatenate(stream,axis=0).astype(np.float32,copy=False)
        size=len(x)
        if size>self.config['context']:
            raise ValueError('Context length exceeded')
        x=x+self.weight('position.weight')[:size]
        heads=self.config['heads']; hwidth=self.config['width']//heads
        for index in range(self.config['layers']):
            pre=f'blocks.layers.{index}'
            z=layernorm(x,self.weight(pre+'.norm1.weight'),self.weight(pre+'.norm1.bias'))
            qkv=z @ self.weight(pre+'.self_attn.in_proj_weight').T + self.weight(pre+'.self_attn.in_proj_bias')
            q,k,v=np.split(qkv,3,axis=-1)
            q=q.reshape(size,heads,hwidth).transpose(1,0,2)
            k=k.reshape(size,heads,hwidth).transpose(1,0,2)
            v=v.reshape(size,heads,hwidth).transpose(1,0,2)
            att=(q @ k.transpose(0,2,1))/np.float32(math.sqrt(hwidth))
            mask=np.triu(np.ones((size,size),dtype=bool),k=1)
            att[:,mask]=-np.inf
            att=att-att.max(axis=-1,keepdims=True)
            exp=np.exp(att)
            att=exp/exp.sum(axis=-1,keepdims=True)
            value=(att @ v).transpose(1,0,2).reshape(size,heads*hwidth)
            x=x+self.linear(value,pre+'.self_attn.out_proj')
            z=layernorm(x,self.weight(pre+'.norm2.weight'),self.weight(pre+'.norm2.bias'))
            x=x+self.linear(gelu(self.linear(z,pre+'.linear1')),pre+'.linear2')
        x=layernorm(x,self.weight('norm.weight'),self.weight('norm.bias'))
        return self.linear(x[prefix:], 'output')

    def generate(self, prompt: str='', image: np.ndarray | None=None,
                 max_new_tokens: int=48, temperature: float=0.0, seed: int=42) -> str:
        if not isinstance(prompt,str): raise TypeError('prompt must be str')
        if not (1<=max_new_tokens<=1024): raise ValueError('bad generation length')
        if temperature < 0: raise ValueError('temperature must be >= 0')
        ids=[BOS]+encode(prompt)
        p=0 if image is None else 1+(self.config['image_size']//self.config['patch_size'])**2
        budget=self.config['context']-len(ids)-p
        if budget<0: raise ValueError('Prompt too long for context')
        out=[]; rng=np.random.default_rng(seed)
        for _ in range(min(max_new_tokens,budget)):
            logits=self.forward(ids,image)[-1].copy()
            logits[[BOS,IMG,PAD]]=-np.inf
            if temperature == 0:
                nxt=int(np.argmax(logits))
            else:
                z=logits/temperature
                z=z-np.max(z)
                probs=np.exp(z); probs=probs/probs.sum()
                nxt=int(rng.choice(self.vocab_size,p=probs))
            if nxt==EOS:break
            ids.append(nxt);out.append(nxt)
        return decode(out)
