"""Guard the held-out split against the contamination that actually happened.

On 2026-10-24 the two held-out books were also present in the training folder.
The earlier evaluation therefore measured books the model had trained on. This
test compares content digests of the held-out files with the files recorded in
a run's local_plan.json and fails if any digest appears in both.

It skips when the local corpus or the run plan is absent, because the data is
deliberately git-ignored and a clean clone has neither.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
HELDOUT = ROOT / "data" / "local" / "gutenberg_heldout"
PLAN = ROOT / "runs" / "local_1m_clean" / "local_plan.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


@pytest.mark.skipif(not HELDOUT.exists() or not PLAN.exists(), reason="local held-out data or run plan is absent")
def test_no_heldout_file_is_in_the_training_plan() -> None:
    plan = json.loads(PLAN.read_text(encoding="utf-8"))
    trained = {entry["sha256"] for entry in plan.get("sources", []) if "sha256" in entry}
    heldout = {path.name: sha256(path) for path in sorted(HELDOUT.glob("*.txt"))}

    assert heldout, "the held-out split is empty"
    overlaps = sorted(name for name, digest in heldout.items() if digest in trained)
    assert overlaps == [], f"held-out files were trained on: {overlaps}"


@pytest.mark.skipif(not HELDOUT.exists() or not PLAN.exists(), reason="local held-out data or run plan is absent")
def test_the_plan_names_every_excluded_heldout_file() -> None:
    plan = json.loads(PLAN.read_text(encoding="utf-8"))
    excluded = " ".join(Path(path).name for path in plan.get("excluded_files", []))
    for path in sorted(HELDOUT.glob("*.txt")):
        assert path.name in excluded, f"{path.name} was not recorded as excluded"
