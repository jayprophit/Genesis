"""Tests for genesis_seed.dataset_pipeline (v0.5)."""
import json
import tempfile
import unittest
from pathlib import Path

import numpy as np

from genesis_seed.dataset_pipeline import (
    DatasetDocument, DatasetManifest, count_tokens, tokenize_bytes,
    ingest_corpus_directory, train_unigram_baseline, train_bigram_baseline,
    generate_unigram, generate_bigram, compute_perplexity,
    deduplicate_documents, normalize_text,
)


class TestDatasetPipeline(unittest.TestCase):
    """Tests for the v0.5 dataset pipeline."""

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.temp_path = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def test_count_tokens(self):
        self.assertEqual(count_tokens("hello world"), 2)
        self.assertEqual(count_tokens(""), 0)
        self.assertEqual(count_tokens("   "), 0)
        self.assertEqual(count_tokens("one"), 1)
        self.assertGreater(count_tokens("long text with several words here"), 1)

    def test_tokenize_bytes(self):
        text = "Hello, Genesis!"
        tokens = tokenize_bytes(text)
        self.assertEqual(tokens, [72, 101, 108, 108, 111, 44, 32, 71, 101, 110, 101, 115, 105, 115, 33])
        self.assertTrue(all(0 <= t <= 255 for t in tokens))

    def test_ingest_corpus_directory(self):
        # Create test corpus
        train_dir = self.temp_path / 'train'
        train_dir.mkdir()
        (train_dir / 'doc1.txt').write_text(
            'Genesis learns from observations. ', encoding='utf-8')
        (train_dir / 'doc2.txt').write_text(
            'Neural networks learn from weights. ', encoding='utf-8')
        (train_dir / 'validation.txt').write_text(
            'Validation documents are held out. ', encoding='utf-8')

        manifest = ingest_corpus_directory(train_dir, self.temp_path / 'manifest.json')

        self.assertEqual(manifest.document_count, 3)
        self.assertEqual(manifest.train_count, 2)
        self.assertEqual(manifest.validation_count, 1)
        self.assertEqual(manifest.tokens_total, 14)
        self.assertGreater(manifest.bytes_total, 0)
        self.assertEqual(manifest.format, 'genesis.corpus.v1')
        self.assertGreater(len(manifest.fingerprint), 10)

    def test_normalize_text(self):
        self.assertEqual(normalize_text('  Hello  World  '), 'hello world')
        self.assertEqual(normalize_text('GENESIS AI'), 'genesis ai')

    def test_deduplicate_documents(self):
        doc1 = DatasetDocument(
            file='a.txt', source='test', license='CC0-1.0', training_permitted=True,
            split='train', content=b'hello world', sha256='abc',
            token_count=2, byte_count=11, dedup_hash='def')
        doc2 = DatasetDocument(
            file='b.txt', source='test', license='CC0-1.0', training_permitted=True,
            split='train', content=b'hello world', sha256='xyz',
            token_count=2, byte_count=11, dedup_hash='def')
        doc3 = DatasetDocument(
            file='c.txt', source='test', license='CC0-1.0', training_permitted=True,
            split='train', content=b'goodbye', sha256='123',
            token_count=1, byte_count=7, dedup_hash='456')

        docs = [doc1, doc2, doc3]
        unique = deduplicate_documents(docs)
        self.assertEqual(len(unique), 2)

    def test_train_unigram_baseline(self):
        corpus = ['hello world', 'world hello', 'hello']
        model = train_unigram_baseline(corpus)
        self.assertGreater(model['vocab_size'], 0)
        self.assertGreater(model['total_tokens'], 0)
        # Each token should have a probability
        self.assertGreater(len(model['probabilities']), 0)

    def test_train_bigram_baseline(self):
        corpus = ['hello world', 'world hello', 'hello again']
        model = train_bigram_baseline(corpus)
        self.assertGreater(model['vocab_size'], 0)
        self.assertGreater(model['total_tokens'], 0)
        self.assertGreater(len(model['bigram_counts']), 0)

    def test_generate_unigram(self):
        corpus = ['hello world', 'world hello', 'hello']
        model = train_unigram_baseline(corpus)
        output = generate_unigram(model, length=5)
        self.assertEqual(len(output.split()), 5)

    def test_generate_bigram(self):
        corpus = ['hello world', 'world hello', 'hello again']
        model = train_bigram_baseline(corpus)
        output = generate_bigram(model, seed='hello', length=3)
        self.assertGreater(len(output.split()), 0)

    def test_compute_perplexity(self):
        corpus = ['hello world', 'world hello', 'hello again']
        model = train_unigram_baseline(corpus)
        pp = compute_perplexity(model, corpus)
        self.assertTrue(pp > 0)

    def test_manifest_roundtrip(self):
        manifest = DatasetManifest(
            name='test_corpus', source='test source', license='CC0-1.0',
            revision='1.0.0', documents=[],
            bytes_total=0, tokens_total=0, document_count=0,
            train_count=0, validation_count=0, test_count=0,
        )
        d = manifest.to_dict()
        restored = DatasetManifest.from_dict(d)
        self.assertEqual(restored.name, 'test_corpus')
        self.assertEqual(restored.fingerprint, '')


if __name__ == '__main__':
    unittest.main()
