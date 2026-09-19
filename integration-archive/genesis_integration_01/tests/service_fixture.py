"""Live v0.5 service fixture (subprocess; integration code stays HTTP-only)."""
from __future__ import annotations

import subprocess
import sys
import time
import urllib.request
from pathlib import Path

V05 = Path("C:/Users/jpowe/Desktop/OpenCode-Agent-Test/agent_bridge_v05")


class LiveService:
    def __init__(self, root: Path, token: str = ""):
        cmd = [sys.executable, "service.py", "--port", "0",
               "--root", str(root)]
        if token:
            cmd += ["--token", token]
        self.proc = subprocess.Popen(
            cmd, cwd=str(V05), stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True)
        line = self.proc.stdout.readline()
        self.base = "http://127.0.0.1:8471"
        if "http://" in line:
            self.base = line.split("serving /v1 on", 1)[1].strip().split()[0]
        end = time.time() + 60
        while time.time() < end:
            try:
                with urllib.request.urlopen(self.base + "/health",
                                            timeout=3) as r:
                    if r.status == 200:
                        return
            except Exception:  # noqa: BLE001
                time.sleep(1)
        self.stop()
        raise RuntimeError("service never became ready: " + line)

    def stop(self) -> None:
        try:
            self.proc.terminate()
            self.proc.wait(timeout=15)
        except Exception:  # noqa: BLE001
            try:
                self.proc.kill()
            except Exception:  # noqa: BLE001
                pass
