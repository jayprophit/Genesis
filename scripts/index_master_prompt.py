"""Generate a manifest of the master-prompt folder for registry tracking."""
from pathlib import Path
import json, hashlib, time

ROOT = Path(__file__).resolve().parents[1]
MASTER = Path.home() / "Documents" / "master prompt"
OUT = ROOT / "data" / "local" / "master-prompt-manifest.jsonl"

records = []
for path in sorted(MASTER.rglob("*")):
    if path.is_file():
        raw = path.read_bytes()
        records.append({
            "path": str(path.relative_to(Path.home() / "Documents")),
            "name": path.name,
            "bytes": len(raw),
            "sha256": hashlib.sha256(raw).hexdigest(),
            "retrieved_at": time.strftime("%Y-%m-%d"),
        })

OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("w", encoding="utf-8") as f:
    for r in records:
        f.write(json.dumps(r, ensure_ascii=False) + "\n")

total_files = len(records)
total_bytes = sum(r["bytes"] for r in records)
print(f"Master prompt manifest: {total_files} files, {total_bytes:,} bytes -> {OUT}")
