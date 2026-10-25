"""Deterministically expand data/sft/tool_use.jsonl with first-party synthetic
tool-use trajectories generated from Genesis-Model's OWN tool registry.

Identity rule: these are synthetic rollouts of the model's own 7 registered tools
(calculator, list_dir, read_file, write_file, search_files, now, http_get). No
third-party weights, no distillation, no borrowed SFT data. Author = Genesis build.

Output: records of {messages:[...], tools:[{name,description}]} in the exact
chat format the existing seed examples use (rendered via apply_chat_template).
Deterministic (fixed seed) so the corpus is reproducible.
"""
from __future__ import annotations
import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEED_SFT = ROOT / "data" / "sft" / "tool_use.jsonl"
OUT = ROOT / "data" / "sft" / "tool_use.jsonl"
SEED = 42

SYSTEM = (
    "You are Genesis, a from-scratch native AI assistant. You answer concisely and, "
    "when a tool is needed, emit exactly one tool call as "
    "<|tool_call|>{\"name\": ..., \"arguments\": {...}}<|/tool_call|>. "
    "After a tool result, give the final answer. Never fabricate a result the tool "
    "did not return."
)


def rec(messages, tools):
    return {"messages": messages, "tools": tools}


def calc(expr, answer):
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"Calculate: {expr}"},
            {"role": "assistant", "tool_calls": [{"name": "calculator", "arguments": {"expression": expr}}]},
            {"role": "tool", "name": "calculator", "content": str(answer)},
            {"role": "assistant", "content": f"The answer is {answer}."},
        ],
        [{"name": "calculator", "description": "Evaluate an arithmetic expression."}],
    )


def listdir(path, entries):
    listing = "\n".join(entries)
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"List the directory: {path}"},
            {"role": "assistant", "tool_calls": [{"name": "list_dir", "arguments": {"path": path}}]},
            {"role": "tool", "name": "list_dir", "content": listing},
            {"role": "assistant", "content": f"{path} contains: {', '.join(entries)}."},
        ],
        [{"name": "list_dir", "description": "List directory entries."}],
    )


def readfile(path, content):
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"Read the file: {path}"},
            {"role": "assistant", "tool_calls": [{"name": "read_file", "arguments": {"path": path}}]},
            {"role": "tool", "name": "read_file", "content": content},
            {"role": "assistant", "content": f"The file {path} says: {content}"},
        ],
        [{"name": "read_file", "description": "Read a text file."}],
    )


def writefile(path, content, approved):
    if approved:
        final = f"The file {path} now contains: {content}"
        tool_content = f"Successfully wrote {len(content)} bytes to {path}"
    else:
        # approval-denial safety case (mirrors the seed example)
        tool_content = "Error: write_file requires approval; approval was not granted."
        final = (
            f"I could not write the note to {path} because the write requires approval "
            f"that was not granted. I will not pretend the file exists."
        )
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"Write this note to {path}: {content}"},
            {"role": "assistant", "tool_calls": [{"name": "write_file", "arguments": {"path": path, "content": content}}]},
            {"role": "tool", "name": "write_file", "content": tool_content},
            {"role": "assistant", "content": final},
        ],
        [{"name": "write_file", "description": "Write content to a file (requires approval)."}],
    )


def searchfiles(pattern, hits):
    listing = "\n".join(hits) if hits else "(no matches)"
    final = (
        f"Files matching {pattern}: {', '.join(hits)}." if hits
        else f"No files matched {pattern}."
    )
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"Find files matching: {pattern}"},
            {"role": "assistant", "tool_calls": [{"name": "search_files", "arguments": {"pattern": pattern}}]},
            {"role": "tool", "name": "search_files", "content": listing},
            {"role": "assistant", "content": final},
        ],
        [{"name": "search_files", "description": "Find files by glob pattern."}],
    )


def now_fn():
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": "What time is it?"},
            {"role": "assistant", "tool_calls": [{"name": "now", "arguments": {}}]},
            {"role": "tool", "name": "now", "content": "2026-10-25T12:00:00Z"},
            {"role": "assistant", "content": "The current time is 2026-10-25T12:00:00Z."},
        ],
        [{"name": "now", "description": "Return the current UTC time."}],
    )


def httpget(url, snippet):
    return rec(
        [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": f"Fetch and summarize: {url}"},
            {"role": "assistant", "tool_calls": [{"name": "http_get", "arguments": {"url": url}}]},
            {"role": "tool", "name": "http_get", "content": snippet},
            {"role": "assistant", "content": f"The page {url} says: {snippet}"},
        ],
        [{"name": "http_get", "description": "Fetch a URL and return its body."}],
    )


def build(rng):
    out = []
    # calculator: varied arithmetic
    for _ in range(70):
        a, b, c = rng.randint(2, 99), rng.randint(2, 99), rng.randint(1, 20)
        op = rng.choice(["+", "-", "*", "//"])
        if op == "//":
            expr, ans = f"{a} {op} {b}", a // b if b else 0
        else:
            expr = f"{a} {op} {b}"
            ans = eval(expr, {"__builtins__": {}}, {})
        # nested expression
        if rng.random() < 0.4:
            d = rng.randint(1, 12)
            expr, ans = f"({expr}) + {c} * {d}", ans + c * d
        out.append(calc(expr, ans))
    # list_dir
    dirs = {
        "C:/Users/jpowe/Desktop/Projects": ["Genesis", "Aetherius", "notes.md"],
        "C:/Users/jpowe/Desktop/Projects/Genesis": ["Genesis-Model", "Genesis-Human-Experiment", "Agent-Bridge"],
        "data/local": ["gutenberg", "sft"],
        "runs/local_1m_gutenberg": ["genesis.pt", "config.json", "local_plan.json"],
    }
    for path in dirs:
        out.append(listdir(path, dirs[path]))
    for _ in range(15):
        n = rng.randint(2, 6)
        entries = [f"file_{i}.txt" for i in range(n)] + (["subdir"] if rng.random() < 0.5 else [])
        out.append(listdir(f"C:/work/dir_{rng.randint(0,9)}", entries))
    # read_file
    for _ in range(20):
        path = f"C:/Users/jpowe/Desktop/Projects/Genesis/Genesis-Model/{rng.choice(['README.md','config.json','notes.txt'])}"
        body = rng.choice([
            "Genesis-Model is a from-scratch byte-level transformer.",
            '{"d_model": 128, "n_layers": 4, "max_seq_len": 256}',
            "This file records the training run configuration.",
        ])
        out.append(readfile(path, body))
    # write_file (success + approval-denial)
    for i in range(25):
        approved = rng.random() < 0.7
        path = f"C:/Users/jpowe/Desktop/notes/todo_{i}.txt"
        content = rng.choice(["buy milk", "finish the SFT run", "review the eval", "write tests"])
        out.append(writefile(path, content, approved))
    # search_files
    for pat in ["*.py", "*.md", "genesis*.pt", "test_*.py", "no_such_file_*"]:
        hits = (
            ["a.py", "b.py", "scripts/c.py"] if pat == "*.py"
            else ["README.md", "docs/gap.md"] if pat == "*.md"
            else ["runs/local_1m_gutenberg/genesis.pt"] if pat == "genesis*.pt"
            else ["test_model.py", "test_eval.py"] if pat == "test_*.py"
            else []
        )
        out.append(searchfiles(pat, hits))
    # now
    for _ in range(5):
        out.append(now_fn())
    # http_get
    for _ in range(15):
        url = rng.choice([
            "https://example.com/", "https://arxiv.org/abs/1804.08404",
            "https://threejs.org/docs/index.html", "https://www.blender.org/",
        ])
        out.append(httpget(url, rng.choice([
            "Example Domain. This page is for documentation and testing.",
            "Abstract: Language Modeling with GPT Transformers.",
            "Three.js - JavaScript 3D library.",
            "Blender, the free and open source 3D creation suite.",
        ])))
    # multi-tool sequences (read then write)
    for i in range(10):
        src = f"C:/Users/jpowe/Desktop/Projects/Genesis/Genesis-Model/notes_{i}.txt"
        dst = f"C:/Users/jpowe/Desktop/backup/notes_{i}.txt"
        body = f"backup note {i}"
        out.append(rec(
            [
                {"role": "system", "content": SYSTEM},
                {"role": "user", "content": f"Copy {src} to {dst}."},
                {"role": "assistant", "tool_calls": [{"name": "read_file", "arguments": {"path": src}}]},
                {"role": "tool", "name": "read_file", "content": body},
                {"role": "assistant", "tool_calls": [{"name": "write_file", "arguments": {"path": dst, "content": body}}]},
                {"role": "tool", "name": "write_file", "content": f"Successfully wrote {len(body)} bytes to {dst}"},
                {"role": "assistant", "content": f"Copied {src} to {dst}."},
            ],
            [
                {"name": "read_file", "description": "Read a text file."},
                {"name": "write_file", "description": "Write content to a file (requires approval)."},
            ],
        ))
    return out


def main():
    rng = random.Random(SEED)
    synthetic = build(rng)
    # keep the existing 5 seed examples first, then append synthetic
    existing = []
    if SEED_SFT.exists():
        for line in SEED_SFT.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if line:
                existing.append(json.loads(line))
    records = existing + synthetic
    OUT.write_text(
        "".join(json.dumps(r, ensure_ascii=False) + "\n" for r in records),
        encoding="utf-8",
    )
    print(f"existing seeds: {len(existing)}  synthetic: {len(synthetic)}  total: {len(records)}")
    # tool coverage
    from collections import Counter
    cov = Counter()
    for r in records:
        for t in r["tools"]:
            cov[t["name"]] += 1
    print("tool coverage:", dict(cov))


if __name__ == "__main__":
    main()
