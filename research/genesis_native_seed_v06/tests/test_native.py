import json
import tempfile
import unittest
from pathlib import Path
import numpy as np
import torch
from genesis_seed.model import GenesisSeed, SeedConfig, BOS, encode
from genesis_seed.data import synthetic_image
from genesis_seed.native_runtime import GenesisNative
from genesis_seed.export_native import export

class TestNativeRuntime(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        torch.set_num_threads(2)
        cls.temp=tempfile.TemporaryDirectory()
        cls.folder=Path(cls.temp.name)
        cls.checkpoint=cls.folder/'source.pt'
        torch.manual_seed(41)
        cls.model=GenesisSeed(SeedConfig(width=32,layers=2)).eval()
        torch.save({'config':cls.model.state_config(),'state_dict':cls.model.state_dict()},cls.checkpoint)
        export(str(cls.checkpoint),str(cls.folder/'native'))
        cls.native=GenesisNative(cls.folder/'native')
    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()
    def test_text_logit_parity(self):
        ids=[BOS]+encode('genesis')
        with torch.no_grad(): reference=self.model(torch.tensor([ids])).numpy()[0]
        actual=self.native.forward(ids)
        np.testing.assert_allclose(reference,actual,rtol=2e-4,atol=3e-5)
    def test_vision_logit_parity(self):
        ids=[BOS]+encode('red')
        image=synthetic_image('red','square',seed=11)
        with torch.no_grad(): reference=self.model(torch.tensor([ids]),image[None]).numpy()[0]
        actual=self.native.forward(ids,image.numpy())
        np.testing.assert_allclose(reference,actual,rtol=2e-4,atol=3e-5)
    def test_deterministic_generation(self):
        ref=self.model.generate(prompt='g',max_new_tokens=6)
        self.assertEqual(ref,self.native.generate(prompt='g',max_new_tokens=6))
    def test_tampered_weights_fail(self):
        weights=self.folder/'native'/'weights.npz'
        before=weights.read_bytes()
        try:
            weights.write_bytes(before+b'X')
            with self.assertRaisesRegex(ValueError,'checksum'): GenesisNative(weights.parent)
        finally: weights.write_bytes(before)
    def test_context_guard(self):
        with self.assertRaises(ValueError): self.native.forward([BOS]*129)

if __name__=='__main__':unittest.main()
