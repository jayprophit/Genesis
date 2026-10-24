import json, hashlib, time
from pathlib import Path
subset_dir = Path(__file__).resolve().parents[1] / "data" / "local" / "master-prompt-subset"
manifest_path = subset_dir / "manifest.jsonl"
records = []
for f in sorted(subset_dir.glob("*.txt")):
    raw = f.read_bytes()
    records.append({
        "name": f.name,
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "retrieved_at": time.strftime("%Y-%m-%d"),
    })
with open(manifest_path, "w", encoding="utf-8") as out:
    for r in records:
        out.write(json.dumps(r, ensure_ascii=False) + "\n")
print("MANIFEST_OK", manifest_path, len(records), sum(r["bytes"] for r in records))
