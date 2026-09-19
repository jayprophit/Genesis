"""Architecture enforcement: integration code uses public APIs only."""
import ast
import sys
import unittest
from pathlib import Path

ROOT = Path(".")
FORBIDDEN = {"executor", "policy", "protocol", "providers", "checkpoints",
             "bridge", "routing", "reviewer", "oracle", "memory", "cache",
             "events", "state", "runtime", "service", "competence", "context",
             "resultkit", "testmap", "progress", "prompts_lib", "versions",
             "config_validate"}
ALLOWED_V05 = {"client"}


def _imports_of(path: Path) -> set[str]:
    tree = ast.parse(path.read_text(encoding="utf-8"))
    found: set[str] = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            found.update(a.name.split(".")[0] for a in node.names)
        elif isinstance(node, ast.ImportFrom) and node.module:
            found.add(node.module.split(".")[0])
    return found


class TestPublicOnly(unittest.TestCase):
    def test_genesis_package_imports(self):
        bad = []
        for py in (ROOT / "genesis_runtime").rglob("*.py"):
            for mod in _imports_of(py) & FORBIDDEN:
                bad.append(f"{py}:{mod}")
        self.assertEqual(bad, [], f"forbidden imports: {bad}")

    def test_drivers_and_simulators(self):
        for name in ("run_synthetic_e2e.py", "run_failure_e2e.py"):
            p = ROOT / name
            if p.exists():
                bad = _imports_of(p) & FORBIDDEN
                self.assertEqual(bad, set(), f"{name}: {bad}")

    def test_only_public_client_used(self):
        used = set()
        for py in (ROOT / "genesis_runtime").rglob("*.py"):
            used |= _imports_of(py) & ALLOWED_V05
        self.assertIn("client", used)

    def test_no_bridge_internals_at_runtime(self):
        import genesis_runtime.v05client as v
        loaded = {m.split(".")[0] for m in sys.modules}
        for mod in FORBIDDEN:
            self.assertNotIn(mod, loaded, f"bridge internal loaded: {mod}")
        self.assertIn("client", loaded)


if __name__ == "__main__":
    unittest.main()
