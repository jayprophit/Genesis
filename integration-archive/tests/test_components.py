"""Component tests: profiles, models, events, status, memory, budgets,
discovery, manifest, secrets, classification, scoping, intent separation."""
import shutil
import tempfile
import unittest
from pathlib import Path

from genesis_runtime import models as M
from genesis_runtime.config import (CHANGE_BUDGETS, PERMISSION_PROFILES,
                                    PRECIOUS_PROJECT_PROFILE)
from genesis_runtime.discovery import (classify_file, discover,
                                       index_summary, scan_secrets)
from genesis_runtime.events import translate, translate_all
from genesis_runtime.memory import cache_info, task_memory
from genesis_runtime.scoping import ChangeBudget, derive_scope
from genesis_runtime.status import approval_view, build_status, verification_view


class TestProfiles(unittest.TestCase):
    def test_known_profiles(self):
        for name in ("SAFE_EXPLORATION", "ASSISTED_BUILD", "AUTONOMOUS_SANDBOX",
                     "PRECIOUS_PROJECT"):
            self.assertIn(name, PERMISSION_PROFILES)
        self.assertEqual(PERMISSION_PROFILES["SAFE_EXPLORATION"]["mode"], "plan")
        self.assertEqual(PERMISSION_PROFILES["SAFE_EXPLORATION"]["approval"],
                         "READ_ONLY")

    def test_precious_locked(self):
        p = PRECIOUS_PROJECT_PROFILE
        self.assertEqual(p["approval"], "ASK_ALL_WRITES")
        self.assertEqual(p["human_gate"], "BEFORE_COMPLETE")
        self.assertEqual(p["network"], "LOCAL_MODEL_NETWORK")
        self.assertEqual(p["permanent_delete"], "elevated-approval-only")

    def test_budgets_conservative(self):
        self.assertLessEqual(
            CHANGE_BUDGETS["PRECIOUS_PROJECT"]["max_files_changed"],
            CHANGE_BUDGETS["AUTONOMOUS_SANDBOX"]["max_files_changed"])


class TestTaskObject(unittest.TestCase):
    def test_defaults_safe(self):
        t = M.GenesisTask(goal="do things", workspace="/tmp/x")
        self.assertEqual(t.approval_profile, "ASSISTED_BUILD")
        self.assertEqual(t.mode, "")

    def test_rejects_empty_goal_and_bad_mode(self):
        with self.assertRaises(ValueError):
            M.GenesisTask(goal="  ")
        with self.assertRaises(ValueError):
            M.GenesisTask(goal="x", mode="turbo")


class TestIntentSeparation(unittest.TestCase):
    def test_destructive_request_is_high_risk_not_authorized(self):
        scope = derive_scope("rewrite everything and delete all files",
                             {"entries": []}, [])
        self.assertEqual(scope["risk"], "high")
        self.assertIn("advisory", scope["note"])

    def test_scope_never_grants(self):
        scope = derive_scope("please patch app.py", {"entries": [
            {"path": "app.py", "excerpt": "patch app now", "kind": "SOURCE"}]}, [])
        self.assertIn("app.py", scope["likely_files"])


class TestEvents(unittest.TestCase):
    def test_translation(self):
        t = translate({"event": "approval.requested", "session_id": "s",
                       "approval_id": "ap-1",
                       "action": {"action": "write", "path": "a"}})
        self.assertEqual(t["genesis_event"], "GENESIS_APPROVAL_REQUIRED")
        self.assertEqual(t["action_id"], "")
        t2 = translate({"event": "execution.completed", "action_id": "a-1",
                        "action": {"action": "write"}, "executed": True,
                        "result": {"ok": True}})
        self.assertEqual(t2["genesis_event"], "GENESIS_FILE_CHANGED")
        t3 = translate({"event": "review.completed", "verdict": "revise"})
        self.assertEqual(t3["genesis_event"], "GENESIS_REVISION_REQUIRED")

    def test_no_fabrication_from_prose(self):
        t = translate({"event": "mystery", "note": "model says done"})
        self.assertEqual(t["genesis_event"], "")

    def test_batch(self):
        out = translate_all([{"event": "task.completed"},
                             {"event": "mystery"}])
        self.assertEqual(len(out), 1)
        self.assertEqual(out[0]["genesis_event"], "GENESIS_COMPLETED")


class TestStatusMemoryVerify(unittest.TestCase):
    def test_status_build(self):
        st = build_status({"session_id": "s", "tasks": {"t": "COMPLETED"},
                           "mode": "build", "step": 3,
                           "files_touched": ["a.py"],
                           "tests": {"ran": 1},
                           "models": {"coder": "m"},
                           "pending_approvals": [], "errors": []},
                          {"runtime": "0.5", "network_policy": "LOCAL"},
                          {"checks": {"ollama": {"status": "HEALTHY"}}})
        self.assertEqual(st.mode, "build")
        self.assertEqual(st.ollama, "HEALTHY")
        self.assertIn("a.py", st.files)

    def test_memory_read_only(self):
        mem = task_memory({"task": "g", "files_touched": ["a"],
                           "tests": {}, "review": "approve", "errors": ["e"]})
        self.assertEqual(mem["goal"], "g")
        self.assertIn("read-only", mem["note"])
        self.assertIn("runtime-managed", cache_info()["status"])

    def test_verification_mapping(self):
        v = verification_view({"scorecard": {"categories": {
            "TESTS": {"status": "PASS", "detail": "ok"}}}})
        self.assertEqual(v["TESTS"]["status"], "PASS")

    def test_approval_cards_bounded(self):
        cards = approval_view([{"id": "ap-1",
                                "action": {"action": "write", "path": "x" * 5000},
                                "context": {"risk": "RISKY", "reason": "r"}}])
        self.assertLess(len(cards[0]["target"]), 600)


class TestDiscovery(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="gdisc_"))
        (self.tmp / "app.py").write_text("print('hi')\n")
        (self.tmp / "tests").mkdir()
        (self.tmp / "tests" / "test_app.py").write_text("assert True\n")
        (self.tmp / ".env").write_text("SECRET_KEY=abcdef123456\n")
        (self.tmp / "notes.md").write_text("# hi\n")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def test_read_only(self):
        before = sorted(p.name for p in self.tmp.iterdir())
        d = discover(self.tmp)
        after = sorted(p.name for p in self.tmp.iterdir())
        self.assertEqual(before, after)
        kinds = {f["path"]: f["kind"] for f in d["files"]}
        self.assertEqual(kinds["app.py"], "SOURCE")
        self.assertEqual(kinds["tests/test_app.py"], "TEST")
        self.assertEqual(kinds["notes.md"], "DOC")

    def test_protected_contents_not_read(self):
        d = discover(self.tmp, protected=[".env"])
        env = [f for f in d["files"] if f["path"] == ".env"][0]
        self.assertTrue(env["protected"])
        self.assertEqual(env["excerpt"], "")
        self.assertEqual(env["secret_warnings"], [])

    def test_manifest_evidence(self):
        d = discover(self.tmp)
        m = d["manifest"]
        self.assertEqual(m["name"], self.tmp.name)
        self.assertIn("python", m["languages"])
        self.assertGreaterEqual(m["file_count"], 4)

    def test_classify(self):
        self.assertEqual(classify_file("a.py"), "SOURCE")
        self.assertEqual(classify_file("t/test_x.py"), "TEST")
        self.assertEqual(classify_file("c.json"), "CONFIG")
        self.assertEqual(classify_file("d.md"), "DOC")
        self.assertEqual(classify_file("x.pyc"), "BINARY")
        self.assertEqual(classify_file("k.py", "api_key = 'abcdef12'"),
                         "SECRET_SUSPECT")

    def test_secrets_never_log_values(self):
        hits = scan_secrets("api_key = 'supersecretvalue123'\npassword = hunter2x\n")
        self.assertEqual({h["kind"] for h in hits}, {"api_key", "password"})
        self.assertNotIn("supersecretvalue123", str(hits))
        self.assertNotIn("hunter2", str(hits))

    def test_bounded_index(self):
        d = discover(self.tmp)
        idx = index_summary(d, task_hint="application tests", max_entries=2)
        self.assertLessEqual(len(idx["entries"]), 2)
        self.assertTrue(idx["total_indexed"] >= 4)


class TestBudgets(unittest.TestCase):
    def test_enforced(self):
        b = ChangeBudget("PRECIOUS_PROJECT")
        ok, msg = b.check_files(["a", "b", "c", "d"], [])
        self.assertFalse(ok)
        self.assertIn("budget", msg)
        ok2, _ = b.check_files(["a"], [])
        self.assertTrue(ok2)
        ok3, _ = b.check_commands(999)
        self.assertFalse(ok3)


if __name__ == "__main__":
    unittest.main()
