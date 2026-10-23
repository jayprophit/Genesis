"""Test configuration.

PyTorch's CPU backend can deadlock when several pytest processes contend for
OpenMP threads on this Windows host. Pin the thread count before torch is
imported so the suite is deterministic and fast.
"""
import os

os.environ.setdefault("OMP_NUM_THREADS", "1")
os.environ.setdefault("MKL_NUM_THREADS", "1")
os.environ.setdefault("NUMEXPR_NUM_THREADS", "1")

import torch  # noqa: E402

torch.set_num_threads(1)

# If a test blocks for more than 30 seconds, print its stack so the hang is
# diagnosable instead of silent.
import faulthandler  # noqa: E402

faulthandler.dump_traceback_later(120, repeat=True)
