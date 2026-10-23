"""Command line interface for Genesis."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from .agent import GenesisAgent, GenesisModelBackend
from .checkpoint import inspect_checkpoint, load_checkpoint
from .config import PRESETS, ModelConfig
from .eval import run_evaluation
from .generate import generate_text
from .server import serve
from .tokenizer import Tokenizer
from .tools import default_registry
from .train import TrainConfig, train


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="genesis-model", description="Genesis first-party model")
    sub = parser.add_subparsers(dest="command", required=True)

    train_p = sub.add_parser("train", help="train a model from random initialisation")
    train_p.add_argument("--preset", choices=sorted(PRESETS), default="tiny")
    train_p.add_argument("--steps", type=int, default=200)
    train_p.add_argument("--batch-size", type=int, default=4)
    train_p.add_argument("--seq-len", type=int, default=128)
    train_p.add_argument("--lr", type=float, default=3e-3)
    train_p.add_argument("--out", default="runs/tiny")
    train_p.add_argument("--corpus", action="append", default=[])
    train_p.add_argument("--sft", action="append", default=[])
    train_p.add_argument("--device", default="auto")
    train_p.add_argument("--seed", type=int, default=42)
    train_p.add_argument("--resume", default=None)

    gen_p = sub.add_parser("generate", help="generate text from a checkpoint")
    gen_p.add_argument("--checkpoint", required=True)
    gen_p.add_argument("--prompt", required=True)
    gen_p.add_argument("--system", default=None)
    gen_p.add_argument("--max-new-tokens", type=int, default=128)
    gen_p.add_argument("--temperature", type=float, default=0.8)
    gen_p.add_argument("--device", default="cpu")

    agent_p = sub.add_parser("agent", help="run the native agent loop")
    agent_p.add_argument("--checkpoint", required=True)
    agent_p.add_argument("--task", required=True)
    agent_p.add_argument("--workspace", default=".")
    agent_p.add_argument("--max-steps", type=int, default=6)
    agent_p.add_argument("--device", default="cpu")

    eval_p = sub.add_parser("eval", help="evaluate a checkpoint")
    eval_p.add_argument("--checkpoint", required=True)
    eval_p.add_argument("--corpus", default=None)
    eval_p.add_argument("--out", default=None)
    eval_p.add_argument("--device", default="cpu")

    inspect_p = sub.add_parser("inspect", help="inspect checkpoint metadata")
    inspect_p.add_argument("--checkpoint", required=True)

    tok_p = sub.add_parser("tokenize", help="train a tokenizer and print a round-trip")
    tok_p.add_argument("--corpus", required=True)
    tok_p.add_argument("--vocab-size", type=int, default=2048)
    tok_p.add_argument("--text", default="Hello, Genesis. 你好，世界。")

    serve_p = sub.add_parser("serve", help="serve the local HTTP runtime")
    serve_p.add_argument("--checkpoint", required=True)
    serve_p.add_argument("--host", default="127.0.0.1")
    serve_p.add_argument("--port", type=int, default=8420)
    serve_p.add_argument("--workspace", default=".")
    serve_p.add_argument("--device", default="cpu")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    if args.command == "train":
        config = TrainConfig(
            steps=args.steps,
            batch_size=args.batch_size,
            seq_len=args.seq_len,
            lr=args.lr,
            out_dir=args.out,
            device=args.device,
            seed=args.seed,
            model=PRESETS[args.preset],
        )
        manifest = train(
            config, corpus_paths=args.corpus or None, sft_paths=args.sft or None,
            resume_from=args.resume,
        )
        print(json.dumps({k: v for k, v in manifest.items() if k != "history"}, indent=2))
        return 0

    if args.command == "generate":
        model, tokenizer, _ = load_checkpoint(args.checkpoint, map_location=args.device)
        model.to(args.device)
        print(generate_text(
            model, tokenizer, args.prompt, system=args.system,
            max_new_tokens=args.max_new_tokens, temperature=args.temperature,
        ))
        return 0

    if args.command == "agent":
        model, tokenizer, _ = load_checkpoint(args.checkpoint, map_location=args.device)
        model.to(args.device)
        backend = GenesisModelBackend(model, tokenizer, temperature=0.3)
        agent = GenesisAgent(backend, default_registry(), workspace=args.workspace,
                             max_steps=args.max_steps)
        result = agent.run(args.task)
        print(json.dumps(result.to_dict(), indent=2, ensure_ascii=False))
        return 0

    if args.command == "eval":
        report = run_evaluation(args.checkpoint, corpus=args.corpus, device=args.device)
        payload = report.to_dict()
        if args.out:
            Path(args.out).write_text(json.dumps(payload, indent=2), encoding="utf-8")
        print(json.dumps(payload, indent=2))
        return 0

    if args.command == "inspect":
        print(json.dumps(inspect_checkpoint(args.checkpoint), indent=2))
        return 0

    if args.command == "tokenize":
        tokenizer = Tokenizer.from_corpus_files([args.corpus], vocab_size=args.vocab_size)
        ids = tokenizer.encode(args.text)
        print(json.dumps({
            "vocab_size": tokenizer.vocab_size,
            "ids": ids,
            "round_trip": tokenizer.decode(ids),
            "exact": tokenizer.decode(ids) == args.text,
        }, indent=2, ensure_ascii=False))
        return 0

    if args.command == "serve":
        serve(args.checkpoint, host=args.host, port=args.port,
              workspace=args.workspace, device=args.device)
        return 0

    return 1


if __name__ == "__main__":
    raise SystemExit(main())
