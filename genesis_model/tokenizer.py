"""Genesis tokenizer: byte-level BPE trained from our own corpus.

Why byte-level:
  * no borrowed vocabulary, no external tokenizer files;
  * every Unicode script (Chinese, Japanese, Korean, Arabic, Indic, Cyrillic,
    emoji) is representable without an unknown token;
  * tool-call and modality boundaries are explicit reserved tokens.

The tokenizer is deliberately dependency-free. Training is O(corpus x merges),
which is appropriate for the 100M-1B token curriculum; a production trainer can
stream and shard without changing the on-disk format.
"""
from __future__ import annotations

import json
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Iterator, Sequence

# Reserved control tokens. Order is part of the format: never reorder.
SPECIAL_TOKENS: tuple[str, ...] = (
    "<|pad|>",
    "<|bos|>",
    "<|eos|>",
    "<|unk|>",
    "<|system|>",
    "<|user|>",
    "<|assistant|>",
    "<|end|>",
    "<|image|>",
    "<|/image|>",
    "<|tool_call|>",
    "<|/tool_call|>",
    "<|tool_result|>",
    "<|/tool_result|>",
    "<|think|>",
    "<|/think|>",
)

PAD = 256
BOS = 257
EOS = 258
UNK = 259
SYSTEM = 260
USER = 261
ASSISTANT = 262
END = 263
IMAGE = 264
IMAGE_END = 265
TOOL_CALL = 266
TOOL_CALL_END = 267
TOOL_RESULT = 268
TOOL_RESULT_END = 269
THINK = 270
THINK_END = 271

BASE_VOCAB = 256
SPECIAL_BASE = 256
BASE_OFFSET = SPECIAL_BASE + len(SPECIAL_TOKENS)  # first merged-token id


@dataclass
class Tokenizer:
    merges: list[tuple[int, int]] = field(default_factory=list)
    special_tokens: tuple[str, ...] = SPECIAL_TOKENS

    def __post_init__(self) -> None:
        self._special_to_id = {
            t: SPECIAL_BASE + i for i, t in enumerate(self.special_tokens)
        }
        self._id_to_special = {i: t for t, i in self._special_to_id.items()}
        # rank of a pair -> index in merges
        self._ranks: dict[tuple[int, int], int] = {
            pair: i for i, pair in enumerate(self.merges)
        }
        # merged id -> (left, right)
        self._expansions: dict[int, tuple[int, int]] = {
            BASE_OFFSET + i: pair for i, pair in enumerate(self.merges)
        }

    # -- properties ---------------------------------------------------------
    @property
    def vocab_size(self) -> int:
        return BASE_OFFSET + len(self.merges)

    def id_of(self, token: str) -> int:
        return self._special_to_id[token]

    # -- training -----------------------------------------------------------
    @classmethod
    def train(
        cls,
        texts: Iterable[str],
        vocab_size: int = 4096,
        *,
        max_documents: int | None = None,
        verbose: bool = False,
    ) -> "Tokenizer":
        if vocab_size <= BASE_OFFSET:
            raise ValueError(f"vocab_size must be > {BASE_OFFSET}")
        docs: list[list[int]] = []
        for i, text in enumerate(texts):
            if max_documents is not None and i >= max_documents:
                break
            docs.append(list(text.encode("utf-8")))
        if not docs:
            raise ValueError("no training text supplied")

        merges: list[tuple[int, int]] = []
        for step in range(vocab_size - BASE_OFFSET):
            counts: Counter[tuple[int, int]] = Counter()
            for doc in docs:
                counts.update(zip(doc, doc[1:]))
            if not counts:
                break
            pair, frequency = counts.most_common(1)[0]
            if frequency < 2:
                break
            merges.append(pair)
            new_id = BASE_OFFSET + len(merges) - 1
            docs = [_merge_doc(doc, pair, new_id) for doc in docs]
            if verbose and (step + 1) % 100 == 0:
                print(f"merge {step + 1}: {pair} freq={frequency} id={new_id}")
        return cls(merges=merges)

    @classmethod
    def from_corpus_files(cls, paths: Sequence[str | Path], vocab_size: int = 4096) -> "Tokenizer":
        def stream() -> Iterator[str]:
            for path in paths:
                yield Path(path).read_text(encoding="utf-8", errors="replace")

        return cls.train(stream(), vocab_size=vocab_size)

    # -- encoding -----------------------------------------------------------
    def encode(self, text: str, *, allow_special: bool = True) -> list[int]:
        """Encode text, recognising reserved tokens when allow_special is true."""
        if not allow_special:
            return self._encode_bytes(text.encode("utf-8"))
        pieces: list[int] = []
        cursor = 0
        # Split on special tokens first; longest match wins.
        order = sorted(self.special_tokens, key=len, reverse=True)
        while cursor < len(text):
            match = None
            for token in order:
                if text.startswith(token, cursor):
                    match = token
                    break
            if match is None:
                next_positions = [
                    p for p in (text.find(t, cursor + 1) for t in order) if p != -1
                ]
                end = min(next_positions) if next_positions else len(text)
                pieces.extend(self._encode_bytes(text[cursor:end].encode("utf-8")))
                cursor = end
            else:
                pieces.append(self._special_to_id[match])
                cursor += len(match)
        return pieces

    def _encode_bytes(self, data: bytes) -> list[int]:
        ids = list(data)
        if not self.merges:
            return ids
        while True:
            best_rank = None
            best_pair = None
            for pair in zip(ids, ids[1:]):
                rank = self._ranks.get(pair)
                if rank is not None and (best_rank is None or rank < best_rank):
                    best_rank = rank
                    best_pair = pair
            if best_pair is None:
                return ids
            ids = _merge_doc(ids, best_pair, BASE_OFFSET + best_rank)

    def decode(self, ids: Iterable[int], *, skip_special: bool = False) -> str:
        out = bytearray()
        for token_id in ids:
            if token_id >= BASE_OFFSET:
                out.extend(self._expand(token_id))
            elif token_id >= BASE_VOCAB:
                if not skip_special:
                    out.extend(self._id_to_special[token_id].encode("utf-8"))
            else:
                out.append(token_id)
        return out.decode("utf-8", errors="replace")

    def _expand(self, token_id: int) -> bytes:
        if token_id < BASE_VOCAB:
            return bytes((token_id,))
        pair = self._expansions.get(token_id)
        if pair is None:
            return b""
        left, right = pair
        return self._expand(left) + self._expand(right)

    # -- chat format --------------------------------------------------------
    def apply_chat_template(
        self,
        messages: Sequence[dict],
        *,
        tools: Sequence[dict] | None = None,
        add_generation_prompt: bool = True,
    ) -> list[int]:
        """Render a conversation into Genesis control tokens.

        Tool schemas are injected as an ordinary system block. The model learns
        to answer with a <|tool_call|> block, which the runtime executes.
        """
        conversation = list(messages)
        if tools:
            schema = json.dumps(list(tools), ensure_ascii=False, separators=(",", ":"))
            preamble = (
                "You are Genesis. Use a tool only when it is needed.\n"
                "To call a tool emit exactly:\n"
                '<|tool_call|>{"name":"tool_name","arguments":{...}}<|/tool_call|>\n'
                "Available tools (JSON Schema):\n" + schema
            )
            if conversation and conversation[0].get("role") == "system":
                conversation[0] = {
                    "role": "system",
                    "content": conversation[0]["content"] + "\n\n" + preamble,
                }
            else:
                conversation.insert(0, {"role": "system", "content": preamble})

        ids: list[int] = [BOS]
        for message in conversation:
            role = message.get("role", "user")
            marker = {
                "system": SYSTEM,
                "user": USER,
                "assistant": ASSISTANT,
                "tool": TOOL_RESULT,
            }.get(role, USER)
            ids.append(marker)
            ids.extend(self.encode(str(message.get("content", ""))))
            ids.append(END if role != "tool" else TOOL_RESULT_END)
        if add_generation_prompt:
            ids.append(ASSISTANT)
        return ids

    # -- persistence --------------------------------------------------------
    def to_dict(self) -> dict:
        return {
            "version": 1,
            "special_tokens": list(self.special_tokens),
            "merges": [[a, b] for a, b in self.merges],
        }

    @classmethod
    def from_dict(cls, data: dict) -> "Tokenizer":
        if data.get("version") != 1:
            raise ValueError("unsupported tokenizer version")
        specials = tuple(data["special_tokens"])
        if specials != SPECIAL_TOKENS:
            raise ValueError("tokenizer special-token layout does not match runtime")
        return cls(merges=[(int(a), int(b)) for a, b in data["merges"]])

    def save(self, path: str | Path) -> None:
        Path(path).write_text(
            json.dumps(self.to_dict(), ensure_ascii=False), encoding="utf-8"
        )

    @classmethod
    def load(cls, path: str | Path) -> "Tokenizer":
        return cls.from_dict(json.loads(Path(path).read_text(encoding="utf-8")))


def _merge_doc(ids: list[int], pair: tuple[int, int], new_id: int) -> list[int]:
    """Merge non-overlapping occurrences of pair, left to right."""
    if not ids:
        return ids
    out: list[int] = []
    i = 0
    first, second = pair
    while i < len(ids):
        if i + 1 < len(ids) and ids[i] == first and ids[i + 1] == second:
            out.append(new_id)
            i += 2
        else:
            out.append(ids[i])
            i += 1
    return out
