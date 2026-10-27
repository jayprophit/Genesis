import os
import tempfile
import unittest
from pathlib import Path
import numpy as np
import torch
from genesis_seed.model import GenesisSeed,SeedConfig,BOS,EOS,encode,decode
from genesis_seed.data import synthetic_batch,synthetic_image,next_token_batch
from genesis_seed.camera import capture_frame
from genesis_seed.bridge import ActionRequest,Authorization,TestEchoProvider,authorized_execute
from genesis_seed.speech import write_wave
from genesis_seed.train import train,load_checkpoint

class TestGenesisSeed(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        torch.set_num_threads(2)
    def test_bytes(self):
        self.assertEqual(decode(encode('Aµ')), 'Aµ')
        self.assertEqual(BOS,256)
    def test_causal_text_and_vision_shapes(self):
        m=GenesisSeed(SeedConfig(width=32,heads=4,layers=1))
        x=torch.tensor([[BOS,*encode('hi')]],dtype=torch.long)
        self.assertEqual(tuple(m(x).shape),(1,3,262))
        image=synthetic_image('red','circle')[None]
        self.assertEqual(tuple(m(x,image).shape),(1,3,262))
    def test_visual_gradient(self):
        m=GenesisSeed(SeedConfig(width=32,heads=4,layers=1))
        x,y,pics=synthetic_batch(2,'cpu',seed=5)
        loss=torch.nn.functional.cross_entropy(m(x,pics).flatten(0,1),y.flatten(),ignore_index=-100)
        loss.backward()
        self.assertGreater(float(m.patch_embedding.weight.grad.abs().sum()),0)
    def test_camera_denied(self):
        with self.assertRaises(PermissionError):capture_frame()
    def test_bridge_denied_and_authorized(self):
        req=ActionRequest(operation='echo',arguments={'msg':'hello'},action_id='a1')
        provider=TestEchoProvider()
        with self.assertRaises(PermissionError):authorized_execute(req,None,provider)
        with self.assertRaises(PermissionError):authorized_execute(req,Authorization('a2',frozenset({'echo'}),True),provider)
        result=authorized_execute(req,Authorization('a1',frozenset({'echo'}),True),provider)
        self.assertEqual(result['arguments']['msg'],'hello')
    def test_wave_created(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'test.wav';write_wave('aeiou',str(p))
            self.assertGreater(p.stat().st_size,1000)
    def test_checkpoint_training_roundtrip(self):
        with tempfile.TemporaryDirectory() as d:
            path=str(Path(d)/'tiny.pt')
            result=train(steps=3,output=path,width=32,log_every=2)
            model=load_checkpoint(path)
            self.assertGreater(Path(path).stat().st_size,100)
            self.assertIsInstance(model.generate(prompt='gen',max_new_tokens=3),str)

if __name__=='__main__':unittest.main()
