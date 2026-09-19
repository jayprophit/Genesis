import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import genesis.core as core


def test_run_task():
    assert core.run_task("x") == "ran x"


def test_version():
    assert core.version() == "0.0.1-synthetic"
