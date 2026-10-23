"""Command line interface for Genesis."""
from __future__ import annotations

import argparse
import json
import os
import platform
from pathlib import Path

import torch

from .agent import GenesisAgent, GenesisModelBackend
from .checkpoint import inspect_checkpoint, load_checkpoint
from .config import PRESETS, ModelConfig, estimate_parameters
from .eval import run_evaluation
from .generate import generate_text
from .server import serve
from .tokenizer import Tokenizer
from .tools import default_registry
from .train import TrainConfig, train


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="genesis-model", description="Genesis first-party model")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("doctor", help="check local CPU/GPU readiness and preset sizes")

    train_p = sub.add_parser("train", help="train a model from random initialisation")
    train_p.add_argument("--preset", choices=sorted(PRESETS), default="tiny")
    train_p.add_argument("--steps", type=int, default=200)
    train_p.add_argument("--batch-size", type=int, default=4)
    train_p.add_argument("--seq-len", type=int, default=128)
    train_p.add_argument("--lr", type=float, default=3e-3)
    train_p.add_argument("--grad-accum", type=int, default=1)
    train_p.add_argument("--save-every", type=int, default=0)
    train_p.add_argument("--max-seconds", type=float, default=0.0)
    train_p.add_argument("--out", default="runs/tiny")
    train_p.add_argument("--corpus", action="append", default=[])
    train_p.add_argument("--sft", action="append", default=[])
    train_p.add_argument("--shard", default=None, help="pre-tokenized memmap token shard")
    train_p.add_argument("--tokenizer", default=None, help="tokenizer.json for the shard")
    train_p.add_argument("--device", default="auto")
    train_p.add_argument("--seed", type=int, default=42)
    train_p.add_argument("--resume", default=None)

    chat_p = sub.add_parser("chat", help="interactive local chat, no API or network")
    chat_p.add_argument("--checkpoint", required=True)
    chat_p.add_argument("--system", default="You are Genesis, a concise local assistant.")
    chat_p.add_argument("--max-new-tokens", type=int, default=200)
    chat_p.add_argument("--temperature", type=float, default=0.7)
    chat_p.add_argument("--device", default="cpu")

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


def _run_doctor() -> int:
    cuda = torch.cuda.is_available()
    info = {
        "platform": platform.platform(),
        "python": platform.python_version(),
        "torch": torch.__version__,
        "cuda_available": cuda,
        "cuda_version": torch.version.cuda,
        "cpu_count": os.cpu_count(),
        "torch_threads": torch.get_num_threads(),
        "working_directory": str(Path.cwd()),
        "local_corpus_folder": str(Path("data/local")),
        "local_corpus_exists": Path("data/local").exists(),
    }
    if cuda:
        props = torch.cuda.get_device_properties(0)
        free, total = torch.cuda.mem_get_info()
        info.update({
            "gpu": props.name,
            "compute_capability": f"{props.major}.{props.minor}",
            "vram_total_mb": round(total / 1024**2, 1),
            "vram_free_mb": round(free / 1024**2, 1),
            "amp": "fp32 (Pascal)" if props.major < 7 else "fp16/bf16",
        })
    info["presets"] = {
        name: {
            "parameters_text_backbone": estimate_parameters(config),
            "context": config.max_seq_len,
            "vision": config.vision is not None,
        }
        for name, config in PRESETS.items()
    }
    info["recommended_local_preset"] = (
        "local_6m" if cuda else "local_1m"
    )
    print(json.dumps(info, indent=2))
    return 0


def _run_chat(args) -> int:
    from .tokenizer import EOS, PAD

    model, tokenizer, meta = load_checkpoint(args.checkpoint, map_location=args.device)
    model.to(args.device)
    device = next(model.parameters()).device
    system = args.system
    history: list[dict] = []
    stop_ids = {EOS, PAD, tokenizer.id_of("<|end|>")}

    print(f"Genesis local chat — {meta.steps} steps, {meta.tokens_seen:,} tokens, device={device}")
    print("Commands: /reset, /exit")
    while True:
        try:
            user = input("you> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not user:
            continue
        if user in {"/exit", "/quit"}:
            break
        if user == "/reset":
            history.clear()
            print("history cleared")
            continue

        messages = ([{"role": "system", "content": system}] if system else [])
        messages += history[-8:]
        messages.append({"role": "user", "content": user})
        prompt_ids = tokenizer.apply_chat_template(messages, add_generation_prompt=True)
        limit = max(32, model.config.max_seq_len - args.max_new_tokens)
        if len(prompt_ids) > limit:
            prompt_ids = prompt_ids[:1] + prompt_ids[-(limit - 1):]

        output = model.generate_ids(
            prompt_ids,
            max_new_tokens=args.max_new_tokens,
            temperature=args.temperature,
            eos_id=EOS,
            forbidden_ids=[PAD],
        )
        new_ids = output[len(prompt_ids):]
        cut = len(new_ids)
        for index, token in enumerate(new_ids):
            if token in stop_ids:
                cut = index
                break
        answer = tokenizer.decode(new_ids[:cut]).strip()
        if not answer:
            answer = "(no answer)"
        print(f"genesis> {answer}")
        if answer != "(no answer)":
            history.append({"role": "user", "content": user})
            history.append({"role": "assistant", "content": answer})
    return 0


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)

    if args.command == "doctor":
        return _run_doctor()

    if args.command == "train":
        config = TrainConfig(
            steps=args.steps,
            batch_size=args.batch_size,
            seq_len=args.seq_len,
            lr=args.lr,
            grad_accum=args.grad_accum,
            save_every=args.save_every,
            max_seconds=args.max_seconds,
            out_dir=args.out,
            device=args.device,
            seed=args.seed,
            shard_path=args.shard,
            tokenizer_path=args.tokenizer,
            model=PRESETS[args.preset],
        )
        manifest = train(
            config, corpus_paths=args.corpus or None, sft_paths=args.sft or None,
            resume_from=args.resume,
        )
        print(json.dumps({k: v for k, v in manifest.items() if k != "history"}, indent=2))
        return 0

    if args.command == "chat":
        return _run_chat(args)

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
