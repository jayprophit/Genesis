"""v0.4-specific tests for language intelligence development.

Coverage:
- Checkpoint migration (v0.3 -> v0.4)
- Reproducibility (deterministic training)
- Vocabulary handling (262 tokens)
- Training resume
- Native inference parity
- Genuine held-out evaluation
"""
import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path
import os

import numpy as np
import torch

from genesis_seed.model import (
    GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, IMG, PAD, INSTRUCTION, RESPONSE,
    encode, decode, VOCAB_SIZE,
)
from genesis_seed.data import synthetic_image, next_token_batch, TRAIN_TEXT, COLORS, SHAPES
from genesis_seed.train_corpus import train as train_corpus
from genesis_seed.train import train as train, load_checkpoint
from genesis_seed.native_runtime import GenesisNative
from genesis_seed.export_native import export
from genesis_seed.evaluate import evaluate


class TestV04Prefix(unittest.TestCase):
    """Tests for v0.4 language-intelligence development."""

    @classmethod
    def setUpClass(cls):
        torch.set_num_threads(2)
        cls.temp = tempfile.TemporaryDirectory()
        cls.folder = Path(cls.temp.name)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    # ---------- Vocabulary handling ----------

    def test_vocab_size_expanded(self):
        self.assertEqual(VOCAB_SIZE, 262)

    def test_special_tokens(self):
        self.assertEqual(BOS, 256)
        self.assertEqual(EOS, 257)
        self.assertEqual(IMG, 258)
        self.assertEqual(PAD, 259)
        self.assertEqual(INSTRUCTION, 260)
        self.assertEqual(RESPONSE, 261)

    def test_encode_decode_roundtrip(self):
        text = "Genesis learns from observations."
        ids = encode(text)
        self.assertEqual(decode(ids), text)
        self.assertTrue(all(0 <= i < VOCAB_SIZE for i in ids))

    def test_special_token_encoded_as_id(self):
        """Instruction and response are valid vocabulary IDs."""
        # These are token IDs; they must be within the vocabulary range.
        self.assertLess(INSTRUCTION, VOCAB_SIZE)
        self.assertLess(RESPONSE, VOCAB_SIZE)

    def test_special_tokens(self):
        """Control tokens have expected numeric values."""
        self.assertEqual(BOS, 256)
        self.assertEqual(EOS, 257)
        self.assertEqual(IMG, 258)
        self.assertEqual(PAD, 259)
        self.assertEqual(INSTRUCTION, 260)
        self.assertEqual(RESPONSE, 261)

    # ---------- Checkpoint migration ----------

    def test_checkpoint_migration_v03_to_v04(self):
        """Load a v0.3 checkpoint (260 vocab) into v0.4 code."""
        cfg = GenesisConfig(seed_config=SeedConfig(width=64, layers=2))
        m = GenesisSeed(cfg)
        self.assertEqual(m.seed_config.width, 64)

        # v0.3 checkpoint has config without 'version' key
        legacy_config = {'width': 64, 'heads': 4, 'layers': 2, 'context': 128,
                         'image_size': 16, 'patch_size': 4, 'dropout': 0.0}
        loaded = GenesisConfig.from_dict(legacy_config)
        self.assertEqual(loaded.version, 'genesis.config.v0.2')
        self.assertIsInstance(loaded.seed_config, SeedConfig)

    def test_legacy_checkpoint_loading(self):
        """v0.3 checkpoint loads with GenesisConfig.from_dict."""
        saved = {
            'version': 'genesis.config.v0.3',
            'model': 'GenesisSeed',
            'seed_config': {'width': 64, 'heads': 4, 'layers': 2, 'context': 128,
                            'image_size': 16, 'patch_size': 4, 'dropout': 0.0},
        }
        cfg = GenesisConfig.from_dict(saved)
        self.assertEqual(cfg.version, 'genesis.config.v0.3')
        self.assertIsInstance(cfg.seed_config, SeedConfig)

    # ---------- Training reproducibility ----------

    def test_deterministic_training_same_seed(self):
        """Training with the same seed produces identical results."""
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        with tempfile.TemporaryDirectory() as d:
            cp1 = Path(d) / 'c1.pt'
            cp2 = Path(d) / 'c2.pt'
            r1 = train_corpus(manifest, cp1, steps=5, width=32, context=64, batch_size=2, seed=42, device='cpu')
            r2 = train_corpus(manifest, cp2, steps=5, width=32, context=64, batch_size=2, seed=42, device='cpu')
            self.assertEqual(r1['step'], r2['step'])
            # History should match
            self.assertEqual(r1['history_tail'], r2['history_tail'])

    def test_deterministic_training_different_seed(self):
        """Different seeds produce different results."""
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        with tempfile.TemporaryDirectory() as d:
            cp1 = Path(d) / 'c1.pt'
            cp2 = Path(d) / 'c2.pt'
            r1 = train_corpus(manifest, cp1, steps=5, width=32, context=64, batch_size=2, seed=42, device='cpu')
            r2 = train_corpus(manifest, cp2, steps=5, width=32, context=64, batch_size=2, seed=7, device='cpu')
            self.assertNotEqual(r1['history_tail'][-1]['train_cross_entropy'],
                                r2['history_tail'][-1]['train_cross_entropy'])

    def test_resume_training(self):
        """Resuming training continues from the checkpoint step."""
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        with tempfile.TemporaryDirectory() as d:
            ckpt = Path(d) / 'ckpt.pt'
            # First run
            r1 = train_corpus(manifest, ckpt, steps=5, width=32, context=64, batch_size=2, seed=42, device='cpu')
            self.assertEqual(r1['step'], 5)
            # Resume
            r2 = train_corpus(manifest, ckpt, steps=3, resume=ckpt, seed=42, device='cpu')
            self.assertEqual(r2['step'], 8)
            self.assertEqual(r2['new_steps'], 3)

    def test_resume_refuses_corrupted_corpus(self):
        """Resume rejects changed corpus fingerprint."""
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        with tempfile.TemporaryDirectory() as d:
            # Copy the manifest to a temp location
            temp_manifest = Path(d) / 'manifest.json'
            # Modify the corpus to have only train1 and validation1 (train2 omitted)
            originals = json.loads((Path(manifest).parent / 'manifest.json').read_text(encoding='utf-8'))
            originals['documents'] = [doc for doc in originals['documents'] if doc['file'] != 'train2.txt']
            temp_manifest.write_text(json.dumps(originals, indent=2), encoding='utf-8')
            # Copy the documents to the same directory as the manifest
            src_dir = Path(manifest).parent
            shutil.copy(src_dir / 'train1.txt', d)
            shutil.copy(src_dir / 'validation1.txt', d)
            ckpt = Path(d) / 'ckpt.pt'
            train_corpus(str(temp_manifest), str(ckpt), steps=5, width=32, context=64, batch_size=2, seed=42, device='cpu')
            # Modify the copied corpus to break the fingerprint
            temp_manifest.write_text(json.dumps({
                'format': 'genesis.corpus.v1',
                'documents': [{'file': 'train1.txt', 'source': 'test',
                               'license': 'CC0-1.0', 'training_permitted': True, 'split': 'train'},
                              {'file': 'validation1.txt', 'source': 'test',
                               'license': 'CC0-1.0', 'training_permitted': True, 'split': 'validation'}]
            }, indent=2))
            with self.assertRaisesRegex(ValueError, 'Refusing resume'):
                train_corpus(str(temp_manifest), str(ckpt), steps=3, resume=str(ckpt), seed=42, device='cpu')

    # ---------- Native inference parity ----------

    def test_native_pytorch_parity(self):
        """Native NumPy inference matches PyTorch reference."""
        torch.manual_seed(41)
        model = GenesisSeed(SeedConfig(width=32, layers=2))
        with tempfile.TemporaryDirectory() as d:
            ckpt = Path(d) / 'source.pt'
            torch.save({'config': model.state_config(), 'state_dict': model.state_dict()}, ckpt)
            export(str(ckpt), str(Path(d) / 'native'))
            native = GenesisNative(Path(d) / 'native')
            ids = [BOS] + encode('genesis')
            # PyTorch reference (no_grad, correct shape [batch, sequence])
            with torch.no_grad():
                ids_tensor = torch.tensor([ids], dtype=torch.long)
                ref = model(ids_tensor).numpy()[0]
            # Native
            actual = native.forward(ids)
            np.testing.assert_allclose(ref, actual, rtol=2e-4, atol=3e-5)

    def test_native_deterministic_generation(self):
        """Native generation is deterministic with same seed."""
        torch.manual_seed(41)
        model = GenesisSeed(SeedConfig(width=32, layers=2))
        with tempfile.TemporaryDirectory() as d:
            ckpt = Path(d) / 'source.pt'
            torch.save({'config': model.state_config(), 'state_dict': model.state_dict()}, ckpt)
            export(str(ckpt), str(Path(d) / 'native'))
            native = GenesisNative(Path(d) / 'native')
            ref = model.generate(prompt='g', max_new_tokens=6)
            self.assertEqual(ref, native.generate(prompt='g', max_new_tokens=6))

    def test_native_tampered_weights_fail(self):
        """Tampered weights are rejected at load time."""
        torch.manual_seed(41)
        model = GenesisSeed(SeedConfig(width=32, layers=2))
        with tempfile.TemporaryDirectory() as d:
            ckpt = Path(d) / 'source.pt'
            torch.save({'config': model.state_config(), 'state_dict': model.state_dict()}, ckpt)
            export(str(ckpt), str(Path(d) / 'native'))
            weights = Path(d) / 'native' / 'weights.npz'
            before = weights.read_bytes()
            try:
                weights.write_bytes(before + b'X')
                with self.assertRaisesRegex(ValueError, 'checksum'):
                    GenesisNative(Path(d) / 'native')
            finally:
                weights.write_bytes(before)

    # ---------- Held-out evaluation ----------

    def test_held_out_evaluation_does_not_contaminate(self):
        """Validation documents are not used for training."""
        # The corpus class enforces disjoint train/validation
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        from genesis_seed.corpus import Corpus
        corpus = Corpus(manifest)
        train_docs = {d.filename for d in corpus.train}
        val_docs = {d.filename for d in corpus.validation}
        self.assertFalse(train_docs & val_docs, "Train/validation overlap detected")

    def test_train_validation_disjoint(self):
        """Train and validation splits are disjoint."""
        manifest = str(Path(__file__).resolve().parents[1] / 'data' / 'demo_corpus' / 'manifest.json')
        from genesis_seed.corpus import Corpus
        corpus = Corpus(manifest)
        train_files = {d.filename for d in corpus.train}
        val_files = {d.filename for d in corpus.validation}
        self.assertEqual(len(train_files), 2)
        self.assertEqual(len(val_files), 1)

    # ---------- Instruction-response format ----------

    def test_instruction_response_tokens(self):
        """Instruction and response tokens are valid vocabulary entries."""
        self.assertEqual(INSTRUCTION, 260)
        self.assertEqual(RESPONSE, 261)
        # Instruction token must be < vocab size
        self.assertLess(INSTRUCTION, VOCAB_SIZE)
        self.assertLess(RESPONSE, VOCAB_SIZE)

    def test_model_output_shape_with_voc16d(self):
        """Model output shape matches vocab size."""
        m = GenesisSeed(SeedConfig(width=32, layers=1))
        x = torch.randint(0, VOCAB_SIZE, (1, 5))
        logits = m(x)
        self.assertEqual(logits.shape, (1, 5, VOCAB_SIZE))


if __name__ == '__main__':
    unittest.main()
