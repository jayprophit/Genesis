from __future__ import annotations
import numpy as np
import torch
from .model import BOS, EOS, PAD, encode

TRAIN_TEXT = [
    'genesis learns from observations.',
    'genesis stores evidence about actions.',
    'a red square is a colored geometric shape.',
    'a green circle is a colored geometric shape.',
    'a blue square is a colored geometric shape.',
    'an observation is not a verified conclusion.',
    'learning updates numerical model weights.',
    'a camera records light from the environment.',
    'a tool action requires authorization.',
    'speech converts text into an audio signal.',
]
COLORS = {'red': (1.,0.,0.), 'green': (0.,1.,0.), 'blue': (0.,0.,1.)}
SHAPES = ('square', 'circle')


def synthetic_image(color: str, shape: str, seed: int = 0, n: int = 16) -> torch.Tensor:
    """Deterministic supervised image data generated in-process, no pretrained encoder."""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[:n,:n]
    cx, cy = n/2 + rng.uniform(-1.1,1.1), n/2 + rng.uniform(-1.1,1.1)
    r = 4.6 + rng.uniform(-0.4,0.4)
    if shape == 'circle':
        mask = (xx - cx)**2 + (yy - cy)**2 <= r**2
    elif shape == 'square':
        mask = (np.abs(xx - cx) <= r) & (np.abs(yy - cy) <= r)
    else:
        raise ValueError('unknown shape')
    if color not in COLORS:
        raise ValueError('unknown color')
    rgb = np.zeros((3,n,n),dtype=np.float32)
    rgb[:,mask] = np.asarray(COLORS[color], dtype=np.float32)[:,None]
    # background noise makes learning less trivial than constant pixel arrays.
    rgb += rng.uniform(0,0.02,rgb.shape).astype(np.float32)
    return torch.from_numpy(rgb.clip(0,1))


def next_token_batch(texts: list[str], device: str) -> tuple[torch.Tensor, torch.Tensor]:
    sequences = [[BOS] + encode(t) + [EOS] for t in texts]
    width = max(len(row) for row in sequences)-1
    x = torch.full((len(sequences),width),PAD,dtype=torch.long,device=device)
    y = torch.full((len(sequences),width),-100,dtype=torch.long,device=device)
    for i,row in enumerate(sequences):
        x[i,:len(row)-1] = torch.tensor(row[:-1],device=device)
        y[i,:len(row)-1] = torch.tensor(row[1:],device=device)
    return x,y


def synthetic_batch(batch_size:int, device:str, seed:int) -> tuple[torch.Tensor,torch.Tensor,torch.Tensor]:
    rng = np.random.default_rng(seed)
    labels, imgs = [], []
    for j in range(batch_size):
        color = list(COLORS)[int(rng.integers(len(COLORS)))]
        shape = SHAPES[int(rng.integers(len(SHAPES)))]
        labels.append(color+' '+shape)
        imgs.append(synthetic_image(color,shape, seed=seed*100+j))
    x,y = next_token_batch(labels, device)
    return x,y,torch.stack(imgs).to(device)
