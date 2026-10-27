import json
import tempfile
import unittest
from pathlib import Path
import torch
from genesis_seed.corpus import Corpus
from genesis_seed.train_corpus import train
from genesis_seed.train import load_checkpoint

class CorpusTests(unittest.TestCase):
    def test_manifest_and_mask(self):
        path=Path(__file__).resolve().parents[1]/'data/demo_corpus/manifest.json'
        corpus=Corpus(str(path))
        self.assertEqual((len(corpus.train),len(corpus.validation)),(2,1))
        a,b=corpus.sample(2,128,'cpu')
        self.assertEqual(a.shape,(2,128))
        self.assertGreater(int((b!=-100).sum()),0)
    def test_missing_permissions(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d)
            (folder/'a.txt').write_text('a')
            (folder/'b.txt').write_text('b')
            items=[{'file':f,'source':'test','license':'original','split':s}
                 for f,s in [('a.txt','train'),('b.txt','validation')]]
            (folder/'manifest.json').write_text(json.dumps({'format':'genesis.corpus.v1','documents':items}))
            with self.assertRaisesRegex(ValueError,'permission'):
                Corpus(str(folder/'manifest.json'))
    def test_resume_and_validate(self):
        torch.set_num_threads(2)
        path=Path(__file__).resolve().parents[1]/'data/demo_corpus/manifest.json'
        with tempfile.TemporaryDirectory() as d:
            checkpoint=str(Path(d)/'genesis.pt')
            first=train(str(path),checkpoint,steps=3,width=32,context=64,batch_size=2)
            second=train(str(path),checkpoint,steps=2,resume=checkpoint)
            self.assertEqual((first['step'],second['step']),(3,5))
            self.assertIsNotNone(load_checkpoint(checkpoint))

if __name__=='__main__':unittest.main()
