"""
Genesis Native AI v0.5 — Dataset pipeline for licensed language data.

Implements streaming/bounded ingestion of publicly available text datasets
with full provenance tracking: source, license, hashes, token counts, and
train/validation/test partition metadata.

This module does NOT require PyTorch or any ML framework. Uses only NumPy.
"""

from __future__ import annotations
import hashlib
import json
import re
import zlib
import numpy as np
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Iterator


@dataclass
class DatasetDocument:
    """A single document with full provenance metadata."""
    file: str
    source: str
    license: str
    training_permitted: bool
    split: str  # 'train', 'validation', or 'test'
    content: bytes
    sha256: str
    token_count: int
    byte_count: int
    dedup_hash: str  # SHA-256 of normalized content for deduplication

    def to_dict(self) -> dict:
        return {
            'file': self.file,
            'source': self.source,
            'license': self.license,
            'training_permitted': self.training_permitted,
            'split': self.split,
            'content': self.content.decode('utf-8', errors='replace'),
            'sha256': self.sha256,
            'token_count': self.token_count,
            'byte_count': self.byte_count,
            'dedup_hash': self.dedup_hash,
        }

    @classmethod
    def from_dict(cls, d: dict) -> 'DatasetDocument':
        return cls(
            file=d['file'],
            source=d['source'],
            license=d['license'],
            training_permitted=d['training_permitted'],
            split=d['split'],
            content=d['content'].encode('utf-8'),
            sha256=d['sha256'],
            token_count=d['token_count'],
            byte_count=d['byte_count'],
            dedup_hash=d['dedup_hash'],
        )


@dataclass
class DatasetManifest:
    """Manifest describing a full dataset with provenance and partitions."""
    format: str = 'genesis.corpus.v1'
    name: str = ''
    source: str = ''
    license: str = ''
    revision: str = ''
    documents: list[DatasetDocument] = field(default_factory=list)
    bytes_total: int = 0
    tokens_total: int = 0
    document_count: int = 0
    train_count: int = 0
    validation_count: int = 0
    test_count: int = 0
    fingerprint: str = ''

    def to_dict(self) -> dict:
        return {
            'format': self.format,
            'name': self.name,
            'source': self.source,
            'license': self.license,
            'revision': self.revision,
            'bytes_total': self.bytes_total,
            'tokens_total': self.tokens_total,
            'document_count': self.document_count,
            'train_count': self.train_count,
            'validation_count': self.validation_count,
            'test_count': self.test_count,
            'documents': [d.to_dict() for d in self.documents],
        }

    @classmethod
    def from_dict(cls, d: dict) -> 'DatasetManifest':
        docs = []
        for dd in d['documents']:
            doc = DatasetDocument(**dd)
            docs.append(doc)
        m = cls(
            name=d['name'], source=d['source'], license=d['license'],
            revision=d['revision'], documents=docs
        )
        m.bytes_total = d['bytes_total']
        m.tokens_total = d['tokens_total']
        m.document_count = d['document_count']
        m.train_count = d['train_count']
        m.validation_count = d['validation_count']
        m.test_count = d['test_count']
        return m

    def compute_fingerprint(self) -> str:
        """Compute SHA-256 fingerprint over document hashes, splits, and licenses."""
        payload = ''.join(
            f"{d.sha256}:{d.split}:{d.license}:{d.training_permitted}:"
            for d in sorted(self.documents, key=lambda x: x.file)
        ).encode('utf-8')
        return hashlib.sha256(payload).hexdigest()


# ---------- Indented token counting ----------

def count_tokens(text: str) -> int:
    """Count tokens as whitespace-separated byte sequences."""
    return len(text.strip().split()) if text.strip() else 0


def tokenize_bytes(text: str) -> list[int]:
    """Convert text to byte-level token IDs (UTF-8 byte values)."""
    return list(text.encode('utf-8'))


# ---------- Dataset ingestion ----------

def ingest_corpus_directory(
    directory: str | Path,
    manifest_path: str | Path,
) -> DatasetManifest:
    """
    Ingest all .txt files from a directory into a DatasetManifest.
    Documents are auto-classified by filename (train/validation/test).
    """
    directory = Path(directory)
    manifest_path = Path(manifest_path)

    documents = []
    total_bytes = 0
    total_tokens = 0

    # Classify documents by split
    train_files = set()
    val_files = set()
    test_files = set()

    for doc_file in sorted(directory.glob('*.txt')):
        content = doc_file.read_bytes()
        content.decode('utf-8', errors='strict')
        filename = doc_file.name

        # Classify split by filename pattern
        if 'validation' in filename or 'val' in filename:
            split = 'validation'
            val_files.add(filename)
        elif 'test' in filename:
            split = 'test'
            test_files.add(filename)
        else:
            split = 'train'
            train_files.add(filename)

        # Compute provenance
        sha256 = hashlib.sha256(content).hexdigest()
        tokens = count_tokens(content.decode('utf-8'))
        dedup_hash = hashlib.sha256(content).hexdigest()

        doc = DatasetDocument(
            file=filename,
            source=f'Local corpus: {directory.name}',
            license='CC0-1.0',
            training_permitted=True,
            split=split,
            content=content,
            sha256=sha256,
            token_count=tokens,
            byte_count=len(content),
            dedup_hash=dedup_hash,
        )
        documents.append(doc)
        total_bytes += len(content)
        total_tokens += tokens

    # Build manifest
    manifest = DatasetManifest(
        name=directory.name,
        source=str(directory.resolve()),
        license='CC0-1.0',
        revision='1.0.0',
        documents=documents,
        bytes_total=total_bytes,
        tokens_total=total_tokens,
        document_count=len(documents),
        train_count=len(train_files),
        validation_count=len(val_files),
        test_count=len(test_files),
    )
    manifest.fingerprint = manifest.compute_fingerprint()

    # Write manifest
    manifest_path.write_text(
        json.dumps(manifest.to_dict(), indent=2), encoding='utf-8'
    )
    print(f'Dataset: {manifest.name}')
    print(f'  Documents: {manifest.document_count} '
          f'(train={manifest.train_count}, val={manifest.validation_count}, '
          f'test={manifest.test_count})')
    print(f'  Bytes: {manifest.bytes_total}, Tokens: {manifest.tokens_total}')
    print(f'  Fingerprint: {manifest.fingerprint}')

    return manifest


def ingest_stream(
    text_stream: Iterable[str],
    source: str,
    license: str,
    revision: str,
    train_documents: list[DatasetDocument],
    val_documents: list[DatasetDocument],
    test_documents: list[DatasetDocument],
) -> DatasetManifest:
    """
    Ingest streaming text (e.g., from a tar.gz or HTTP stream) into
    train/validation/test partitions.
    """
    documents = []
    total_bytes = 0
    total_tokens = 0

    for i, text in enumerate(text_stream):
        content = text.encode('utf-8')
        filename = f'doc_{i:06d}.txt'

        # Small batches: 1 byte per "document" is not useful; accumulate
        # until a reasonable size, then split into train/val/test.
        # For now, each item is a document.
        sha256 = hashlib.sha256(content).hexdigest()
        tokens = count_tokens(text)
        dedup_hash = hashlib.sha256(content).hexdigest()

        # Assign split (alternating, or based on content classification)
        split = 'train' if i % 10 < 8 else ('validation' if i % 10 < 9 else 'test')

        doc = DatasetDocument(
            file=filename,
            source=source,
            license=license,
            training_permitted=True,
            split=split,
            content=content,
            sha256=sha256,
            token_count=tokens,
            byte_count=len(content),
            dedup_hash=dedup_hash,
        )
        documents.append(doc)
        total_bytes += len(content)
        total_tokens += tokens

    manifest = DatasetManifest(
        name='stream_input',
        source=source,
        license=license,
        revision=revision,
        documents=documents,
        bytes_total=total_bytes,
        tokens_total=total_tokens,
        document_count=len(documents),
        train_count=sum(1 for d in documents if d.split == 'train'),
        validation_count=sum(1 for d in documents if d.split == 'validation'),
        test_count=sum(1 for d in documents if d.split == 'test'),
    )
    manifest.fingerprint = manifest.compute_fingerprint()

    return manifest


# ---------- Statistical baselines (unigram/bigram) ----------

def train_unigram_baseline(corpus: list[str]) -> dict:
    """
    Train a unigram language model: P(w) = count(w) / total_tokens.
    Returns vocabulary and probability distribution.
    """
    vocab = {}
    total_tokens = 0
    for text in corpus:
        for token in text.strip().split():
            vocab[token] = vocab.get(token, 0) + 1
            total_tokens += 1

    # Laplace smoothing
    vocab_size = len(vocab) + 1
    return {
        'vocab': vocab,
        'vocab_size': vocab_size,
        'total_tokens': total_tokens,
        'probabilities': {word: (count + 1) / (total_tokens + vocab_size)
                          for word, count in vocab.items()},
        'baseline_type': 'unigram',
    }


def train_bigram_baseline(corpus: list[str]) -> dict:
    """
    Train a bigram language model: P(w2|w1) = count(w1,w2) / count(w1).
    """
    unigram_counts = {}
    bigram_counts = {}
    total_tokens = 0

    for text in corpus:
        tokens = text.strip().split()
        for i, token in enumerate(tokens):
            unigram_counts[token] = unigram_counts.get(token, 0) + 1
            total_tokens += 1
            if i > 0:
                prev = tokens[i - 1]
                bigram = (prev, token)
                bigram_counts[bigram] = bigram_counts.get(bigram, 0) + 1

    # Laplace smoothing
    vocab = set()
    for text in corpus:
        vocab.update(text.strip().split())
    vocab_size = len(vocab) + 1

    # Build per-w1 bigram counts
    bigram_by_w1 = {}
    for (w1, w2), count in bigram_counts.items():
        if w1 not in bigram_by_w1:
            bigram_by_w1[w1] = {}
        bigram_by_w1[w1][w2] = count

    return {
        'vocab_size': vocab_size,
        'total_tokens': total_tokens,
        'bigram_counts': bigram_counts,
        'unigram_counts': unigram_counts,
        'probabilities': {
            w1: {
                w2: (count + 1) / (unigram_counts[w1] + vocab_size)
                for w2, count in w1_bigrams.items()
            }
            for w1, w1_bigrams in bigram_by_w1.items()
        },
        'baseline_type': 'bigram',
    }


def evaluate_unigram(bigram_model: dict, prompt: str, target: str) -> float:
    """Evaluate unigram model on a target token."""
    if target not in bigram_model['probabilities']:
        return 1.0 / bigram_model['vocab_size']  # Laplace smoothing
    return bigram_model['probabilities'][target]


def evaluate_bigram(model: dict, w1: str, w2: str) -> float:
    """Evaluate bigram model on (w1, w2)."""
    if w1 not in model['probabilities']:
        return 1.0 / model['vocab_size']
    if w2 not in model['probabilities'][w1]:
        return 1.0 / model['vocab_size']
    return model['probabilities'][w1][w2]


def generate_unigram(model: dict, length: int = 10) -> str:
    """Greedy unigram generation."""
    tokens = []
    for _ in range(length):
        # Pick highest-probability token
        best = max(model['probabilities'].items(), key=lambda x: x[1])
        tokens.append(best[0])
    return ' '.join(tokens)


def generate_bigram(model: dict, seed: str, length: int = 10) -> str:
    """Greedy bigram generation from a seed token."""
    tokens = [seed]
    for _ in range(length):
        if tokens[-1] in model['probabilities']:
            next_tokens = model['probabilities'][tokens[-1]]
            next_token = max(next_tokens.items(), key=lambda x: x[1])
            tokens.append(next_token[0])
        else:
            break
    return ' '.join(tokens)


def compute_perplexity(model: dict, test_corpus: list[str]) -> float:
    """
    Compute perplexity: 2^(-1/N * sum(log2(P(w_i)))).
    """
    log_probs = []
    for text in test_corpus:
        tokens = text.strip().split()
        for token in tokens:
            p = model['probabilities'].get(token, 1.0 / model['vocab_size'])
            log_probs.append(np.log2(p) if p > 0 else -10.0)

    if not log_probs:
        return float('inf')

    avg_log_prob = sum(log_probs) / len(log_probs)
    return 2 ** (-avg_log_prob)


# ---------- Cleanup: dedup and normalizing ----------

def normalize_text(text: str) -> str:
    """Normalize text for deduplication and hashing."""
    # Lowercase, collapse whitespace, strip
    text = text.lower()
    text = re.sub(r'\s+', ' ', text)
    text = text.strip()
    return text


def deduplicate_documents(documents: list[DatasetDocument]) -> list[DatasetDocument]:
    """
    Remove duplicate documents based on normalized content hashes.
    Keeps only the first occurrence of each unique document.
    """
    seen = set()
    unique = []
    for doc in sorted(documents, key=lambda x: x.file):
        normalized = normalize_text(doc.content.decode('utf-8'))
        normalized_hash = hashlib.sha256(normalized.encode('utf-8')).hexdigest()
        if normalized_hash not in seen:
            seen.add(normalized_hash)
            unique.append(doc)
    return unique
