"""Text generation helpers for Genesis."""
from __future__ import annotations

from typing import Callable, Iterable

import torch

from .model import GenesisModel
from .tokenizer import EOS, Tokenizer


def generate_text(
    model: GenesisModel,
    tokenizer: Tokenizer,
    prompt: str,
    *,
    system: str | None = None,
    max_new_tokens: int = 128,
    temperature: float = 0.8,
    top_k: int | None = 50,
    top_p: float = 0.95,
    images: torch.Tensor | None = None,
    echo_prompt: bool = False,
) -> str:
    messages = []
    if system:
        messages.append({"role": "system", "content": system})
    messages.append({"role": "user", "content": prompt})
    ids = tokenizer.apply_chat_template(messages, add_generation_prompt=True)
    output = model.generate_ids(
        ids,
        max_new_tokens=max_new_tokens,
        temperature=temperature,
        top_k=top_k,
        top_p=top_p,
        eos_id=EOS,
        images=images,
        forbidden_ids=[tokenizer.id_of("<|pad|>")],
    )
    cut = len(output)
    for index, token in enumerate(output):
        if token in (EOS, tokenizer.id_of("<|end|>")):
            cut = index
            break
    text = tokenizer.decode(output[:cut])
    if echo_prompt:
        return tokenizer.decode(ids) + text
    return text.strip()


def stream_text(
    model: GenesisModel,
    tokenizer: Tokenizer,
    prompt: str,
    on_token: Callable[[str], None],
    **kwargs,
) -> str:
    """Generate and call on_token with each decoded chunk as it is produced.

    Kept simple for the local runtime: a token is one byte-BPE piece, so chunks
    can split a multi-byte character. The caller accumulates bytes and decodes
    incrementally in the UI.
    """
    text = generate_text(model, tokenizer, prompt, **kwargs)
    for character in text:
        on_token(character)
    return text
