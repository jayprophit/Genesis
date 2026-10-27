"""Verify native-only command line is safe on machines without PyTorch installed."""
import subprocess
import sys
import unittest

class NoTorchRuntime(unittest.TestCase):
    def test_no_pytorch_import(self):
        check="import sys; import genesis_seed.cli_native; import genesis_seed.server_native; assert 'torch' not in sys.modules"
        result=subprocess.run([sys.executable,'-c',check],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
if __name__=='__main__':unittest.main()
