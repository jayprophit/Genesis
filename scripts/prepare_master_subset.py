"""Prepare a Genesis-relevant subset of the master-prompt folder for training."""
from pathlib import Path
import json, hashlib, shutil, time

MASTER = Path.home() / "Documents" / "master prompt"
SUBSET_DIR = Path(__file__).resolve().parents[1] / "data" / "local" / "master-prompt-subset"
MANIFEST = SUBSET_DIR / "manifest.jsonl"

KEYWORDS = ["GENESIS", "MASTER CHAT", "MASTER CONVERSATION HANDOFF", "MASTER PROJECT", "HANDOFF",
            "master execution", "master spec", "AGENT BRIDGE", "NANOBRAIN", "DIGITAL ORGANISM"]

matches = []
for f in MASTER.rglob("*"):
    if f.is_file() and any(k in f.name.upper() for k in KEYWORDS):
        matches.append(f)

matches.sort(key=lambda p: p.name)

SUBSET_DIR.mkdir(parents=True, exist_ok=True)
records = []
for src in matches:
    dst = SUBSET_DIR / src.name
    shutil.copy2(str(src), str(dst))
    raw = dst.read_bytes()
    records.append({
        "name": src.name,
        "source_path": str(src.relative_to(Path.home() / "Documents")),
        "subset_path": str(dst.relative_to(Path(__file__).resolve().parents[1])),
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "retrieved_at": time.strftime("%Y-%m-%d"),
    })
    print(f"{src.name}: {len(raw):,} bytes -> {dst}")

with MANIFEST.open("w", encoding="utf-8") as f:
    for r in records:
        f.write(json.dumps(r, ensure_ascii=False) + "\n")

total_bytes = sum(r["bytes"] for r in records)
print(f"Subset manifest: {len(records)} files, {total_bytes:,} bytes -> {MANIFEST}")
