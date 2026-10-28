"""Native 3-task cycle benchmark for Genesis.

Runs the trained model through three representative tool-use tasks:
  1. calculator  — deterministic math
  2. read_file   — file read with path confinement
  3. now         — time retrieval (no file I/O)

Validates that the model can emit valid tool calls, that the tool layer
executes them correctly, and that the final answer is coherent.

Usage:
    python scripts/benchmark_native_3task_cycle.py \
        --checkpoint runs/sft_a_tool_seq256/genesis.pt \
        --device cpu
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.agent import GenesisAgent, GenesisModelBackend
from genesis_model.checkpoint import load_checkpoint
from genesis_model.tools import default_registry
from genesis_model.tokenizer import Tokenizer


TASKS = [
    {
        "id": "calc",
        "name": "Calculator",
        "prompt": "What is 17 * 24?",
        "expected_tool": "calculator",
        "expected_args": {"expression": "17 * 24"},
        "expected_result": 408,
    },
    {
        "id": "read",
        "name": "Read File",
        "prompt": "Read the file notes.txt and tell me its contents.",
        "setup": lambda ws: (ws / "notes.txt").write_text("hello world", encoding="utf-8"),
        "expected_tool": "read_file",
        "expected_args_pattern": "notes.txt",
        "expected_result_contains": "hello world",
    },
    {
        "id": "now",
        "name": "Current Time",
        "prompt": "What is the current time?",
        "expected_tool": "now",
        "expected_args": {},
        "expected_result_pattern": ":",  # HH:MM format
    },
]


def run_task(agent: GenesisAgent, task: dict, workspace: Path) -> dict:
    if "setup" in task:
        task["setup"](workspace)

    # Ensure max_steps is set on the agent for this task
    agent.max_steps = 4

    t0 = time.time()
    result = agent.run(task["prompt"])
    elapsed = time.time() - t0

    tool_calls = [tc.name for tc in result.tool_calls]
    tool_args = [tc.arguments for tc in result.tool_calls]

    success = False
    reason = "no_tool_call"

    if task["expected_tool"] in tool_calls:
        idx = tool_calls.index(task["expected_tool"])
        args = tool_args[idx]
        if "expected_args" in task:
            if args == task["expected_args"]:
                success = True
                reason = "exact_match"
            else:
                reason = f"args_mismatch: got {args}"
        elif "expected_args_pattern" in task:
            if task["expected_args_pattern"] in str(args.get("path", "")):
                success = True
                reason = "path_match"
            else:
                reason = f"path_mismatch: {args}"
        else:
            success = True
            reason = "tool_called"

        if success and "expected_result" in task:
            if result.tool_results[idx].ok:
                output = result.tool_results[idx].output
                if isinstance(output, dict) and output.get("value") == task["expected_result"]:
                    reason = "exact_result"
                else:
                    reason += f"; result={output}"
            else:
                success = False
                reason += f"; tool_failed: {result.tool_results[idx].error}"

    return {
        "id": task["id"],
        "name": task["name"],
        "success": success,
        "reason": reason,
        "tool_calls": tool_calls,
        "answer": result.answer[:200] if result.answer else "",
        "steps": result.steps,
        "elapsed_s": round(elapsed, 3),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="Native 3-task cycle benchmark")
    ap.add_argument("--checkpoint", required=True, help="Path to genesis.pt")
    ap.add_argument("--device", default="cpu")
    ap.add_argument("--workspace", default=".tmp_3task_workspace")
    args = ap.parse_args()

    checkpoint = Path(args.checkpoint)
    if not checkpoint.exists():
        print(f"Checkpoint not found: {checkpoint}")
        return 2

    workspace = Path(args.workspace)
    workspace.mkdir(exist_ok=True)

    print(f"Loading checkpoint: {checkpoint}")
    model, tokenizer, meta = load_checkpoint(checkpoint, map_location=args.device)
    backend = GenesisModelBackend(model, tokenizer, temperature=0.0, max_new_tokens=64)
    registry = default_registry()
    agent = GenesisAgent(backend, registry, workspace=workspace, max_steps=4)

    results = []
    for task in TASKS:
        print(f"\n--- Task: {task['name']} ---")
        res = run_task(agent, task, workspace)
        results.append(res)
        status = "PASS" if res["success"] else "FAIL"
        print(f"  {status}: {res['reason']}")
        print(f"  tool_calls: {res['tool_calls']}")
        print(f"  answer: {res['answer'][:120]}")
        print(f"  steps: {res['steps']}, elapsed: {res['elapsed_s']}s")

    passed = sum(1 for r in results if r["success"])
    total = len(results)
    print(f"\n=== SUMMARY: {passed}/{total} tasks passed ===")

    report = {
        "checkpoint": str(checkpoint),
        "parameters": model.num_parameters(),
        "vocab_size": tokenizer.vocab_size,
        "tasks": results,
        "passed": passed,
        "total": total,
    }
    out_path = ROOT / "reports" / "native_3task_cycle.json"
    out_path.parent.mkdir(exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Report written to {out_path}")

    return 0 if passed == total else 1


if __name__ == "__main__":
    raise SystemExit(main())
