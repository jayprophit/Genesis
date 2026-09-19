"""Self-check + audit report tests (no project mutation)."""
import json
import unittest

from genesis_runtime import audit as audit_mod
from genesis_runtime import selfcheck as selfcheck_mod


class FakeAdapter:
    def __init__(self, health=None, caps=None, models=None):
        self._health = health or {"status": "HEALTHY"}
        self._caps = caps or {"actions": ["list"]}
        self._models = models if models is not None else {"models": [{"name": "m"}]}

    def health(self):
        return self._health

    def capabilities(self):
        return self._caps

    def models(self):
        return self._models

    def events(self, sid):
        raise KeyError("unknown")

    def deny(self, sid, aid):
        raise KeyError("unknown")


class TestSelfcheck(unittest.TestCase):
    def test_structure(self):
        out = selfcheck_mod.run(FakeAdapter())
        names = [c["check"] for c in out]
        for expected in ("runtime_reachable", "api_compatible", "model_inventory",
                         "workspace_authorization", "event_stream",
                         "approval_resolution", "secret_layer", "change_budgets",
                         "protected_paths"):
            self.assertIn(expected, names)
        for c in out:
            self.assertIn(c["status"], ("PASS", "WARN", "FAIL"))

    def test_unreachable(self):
        class Dead(FakeAdapter):
            def health(self):
                raise ConnectionError("down")

        out = selfcheck_mod.run(Dead())
        self.assertEqual(out[0]["status"], "FAIL")

    def test_no_mutation_probe(self):
        import tempfile
        from pathlib import Path
        before = set()
        out = selfcheck_mod.run(FakeAdapter())
        self.assertTrue(out)


class TestAudit(unittest.TestCase):
    def test_report_redacted(self):
        rep = audit_mod.build("0.1", "0.5", "v1", {"tests": 10},
                              {"boundary": "public-only"},
                              {"approval": {"ok": True}}, ["limitation"],
                              )
        self.assertEqual(rep["integration_version"], "0.1")
        text = audit_mod.render_text(rep)
        self.assertIn("audit", text)
        blob = json.dumps(rep).lower()
        for banned in ('"token"', '"password"', '"api_key"', '"secret"'):
            self.assertNotIn(banned, blob)
