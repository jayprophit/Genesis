"""Fetch a small, curated public-domain corpus from Project Gutenberg.

This is intentionally a *small* list of long-established works. Every selected
author and translator died before 1947, so the texts are public domain in
life+70 jurisdictions, and the editions used here are hosted by Project
Gutenberg US. Works first published after 1928 are excluded.

The Project Gutenberg header and footer are removed before training. The raw
download and the cleaned text are hashed and recorded in a JSONL manifest.

Usage:
    python scripts/fetch_gutenberg.py
    python scripts/fetch_gutenberg.py --ids 1342 84 11

Network access is required. Files go to ``data/local/`` which is git-ignored.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RAW_DIR = ROOT / "data" / "local" / "gutenberg_raw"
CLEAN_DIR = ROOT / "data" / "local" / "gutenberg"
MANIFEST = ROOT / "data" / "local" / "gutenberg_manifest.jsonl"

# Curated classics with verified Project Gutenberg IDs. Author and translator
# death years are before 1947, and the editions are public domain in the US.
# The comments describe the actual file at each ID, not an intended title.
BOOK_IDS = [
    11,     # Alice's Adventures in Wonderland (Carroll)
    12,     # Through the Looking-Glass (Carroll)
    23,     # Narrative of the Life of Frederick Douglass (Douglass)
    35,     # The Time Machine (Wells)
    36,     # The War of the Worlds (Wells)
    43,     # Dr Jekyll and Mr Hyde (Stevenson)
    45,     # Anne of Green Gables (Montgomery)
    47,     # Anne of Avonlea (Montgomery)
    51,     # Anne of the Island (Montgomery)
    55,     # The Wonderful Wizard of Oz (Baum)
    61,     # The Communist Manifesto (Marx/Engels; Moore translation)
    74,     # Tom Sawyer (Twain)
    76,     # Huckleberry Finn (Twain)
    84,     # Frankenstein (Shelley)
    98,     # A Tale of Two Cities (Dickens)
    100,    # The Complete Works of Shakespeare
    105,    # Persuasion (Austen)
    113,    # The Secret Garden (Burnett)
    120,    # Treasure Island (Stevenson)
    121,    # Northanger Abbey (Austen)
    158,    # Emma (Austen)
    161,    # Sense and Sensibility (Austen)
    174,    # The Picture of Dorian Gray (Wilde)
    215,    # White Fang (London)
    244,    # A Study in Scarlet (Doyle)
    245,    # Life on the Mississippi (Twain)
    421,    # Kidnapped (Stevenson)
    427,    # The Great War Syndicate (Stockton)
    520,    # The Life and Adventures of Santa Claus (Baum)
    523,    # Court Life in China (Headland)
    582,    # A Collection of Beatrix Potter Stories (Potter)
    768,    # Wuthering Heights (Emily Bronte)
    829,    # Gulliver's Travels (Swift)
    880,    # My Garden Acquaintance (Lowell)
    946,    # Lady Susan (Austen)
    969,    # The Tenant of Wildfell Hall (Anne Bronte)
    1013,   # The First Men in the Moon (Wells)
    1080,   # A Modest Proposal (Swift)
    1184,   # The Count of Monte Cristo (Dumas)
    1228,   # On the Origin of Species (Darwin)
    1232,   # The Prince (Machiavelli; Marriott translation)
    1260,   # Jane Eyre (Charlotte Bronte)
    1342,   # Pride and Prejudice (Austen)
    1399,   # Anna Karenina (Tolstoy; Garnett translation)
    1400,   # Great Expectations (Dickens)
    1497,   # The Republic (Plato; Jowett translation)
    1661,   # The Adventures of Sherlock Holmes (Doyle)
    1952,   # The Yellow Wallpaper (Gilman)
    1998,   # Thus Spake Zarathustra (Nietzsche; Common translation)
    2097,   # The Sign of the Four (Doyle)
    2148,   # The Works of Edgar Allan Poe, volume 2
    2542,   # A Doll's House (Ibsen; Archer/Gosse translation)
    2554,   # Crime and Punishment (Dostoevsky; Garnett translation)
    2591,   # Grimms' Fairy Tales (Grimm; Hunt translation)
    2600,   # War and Peace (Tolstoy; Garnett translation)
    2701,   # Moby Dick (Melville)
    2852,   # The Hound of the Baskervilles (Doyle)
    3207,   # Leviathan (Hobbes)
    3300,   # The Wealth of Nations (Smith)
]

USER_AGENT = "Genesis-Model/0.1 (local public-domain corpus; contact: local user)"
LICENCE = (
    "Public domain: author and translator died before 1947; "
    "Project Gutenberg US edition published before 1929"
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fetch(book_id: int, retries: int = 3) -> tuple[bytes, str]:
    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"
    last_error: Exception | None = None
    for attempt in range(retries):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(request, timeout=60) as response:
                return response.read(), url
        except (urllib.error.URLError, TimeoutError, OSError) as exc:
            last_error = exc
            time.sleep(1.5 * (attempt + 1))
    raise RuntimeError(f"download failed for {url}: {last_error}")


def parse_metadata(text: str) -> dict[str, str]:
    metadata: dict[str, str] = {}
    for line in text.splitlines()[:100]:
        if line.startswith("Title:"):
            metadata["title"] = line.split(":", 1)[1].strip()
        elif line.startswith("Author:"):
            metadata["author"] = line.split(":", 1)[1].strip()
        elif line.startswith("Translator:"):
            metadata["translator"] = line.split(":", 1)[1].strip()
        elif line.startswith("Language:"):
            metadata["language"] = line.split(":", 1)[1].strip()
    return metadata


def clean_text(raw: bytes) -> tuple[str, dict[str, str]]:
    text = raw.decode("utf-8", errors="replace").replace("\r\n", "\n")
    metadata = parse_metadata(text)
    start_marker = "*** START OF THE PROJECT GUTENBERG EBOOK"
    end_marker = "*** END OF THE PROJECT GUTENBERG EBOOK"
    start = text.find(start_marker)
    end = text.find(end_marker, start + 1 if start >= 0 else 0)
    if start < 0 or end < 0:
        raise ValueError("Gutenberg start/end markers not found")
    body = text[text.find("\n", start) + 1:end].strip()
    if len(body) < 5_000:
        raise ValueError(f"cleaned body is too short ({len(body)} chars)")
    return body + "\n", metadata


def make_record(
    book_id: int, url: str, raw: bytes, body: str, metadata: dict[str, str]
) -> dict[str, object]:
    return {
        "id": f"gutenberg-{book_id}",
        "source": f"https://www.gutenberg.org/ebooks/{book_id}",
        "url": url,
        "title": metadata.get("title", ""),
        "author": metadata.get("author", ""),
        "translator": metadata.get("translator", ""),
        "language": metadata.get("language", ""),
        "licence": LICENCE,
        "raw_sha256": sha256(raw),
        "clean_sha256": sha256(body.encode("utf-8")),
        "raw_bytes": len(raw),
        "clean_chars": len(body),
        "retrieved_at": time.strftime("%Y-%m-%d"),
        "modality": "text",
        "allowed_use": "training",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ids", nargs="*", type=int, default=None)
    parser.add_argument("--force", action="store_true", help="re-download and rewrite")
    parser.add_argument("--delay", type=float, default=0.4)
    args = parser.parse_args()

    ids = args.ids if args.ids is not None else BOOK_IDS
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    CLEAN_DIR.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, object]] = []

    for index, book_id in enumerate(ids, start=1):
        raw_path = RAW_DIR / f"pg{book_id}.txt"
        clean_path = CLEAN_DIR / f"pg{book_id}.txt"

        if clean_path.exists() and raw_path.exists() and not args.force:
            raw = raw_path.read_bytes()
            try:
                body, metadata = clean_text(raw)
            except ValueError as exc:
                print(f"[{index}/{len(ids)}] SKIP cached {book_id}: {exc}", flush=True)
                continue
            records.append(
                make_record(
                    book_id,
                    f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt",
                    raw,
                    body,
                    metadata,
                )
            )
            print(f"[{index}/{len(ids)}] cached {book_id}: {len(body):,} chars", flush=True)
            continue

        try:
            raw, url = fetch(book_id)
            body, metadata = clean_text(raw)
        except Exception as exc:  # noqa: BLE001 - report and continue the batch
            print(f"[{index}/{len(ids)}] SKIP {book_id}: {exc}", flush=True)
            continue

        raw_path.write_bytes(raw)
        clean_path.write_text(body, encoding="utf-8")
        records.append(make_record(book_id, url, raw, body, metadata))
        print(
            f"[{index}/{len(ids)}] {book_id}: {len(raw):,} raw bytes -> "
            f"{len(body):,} chars | {metadata.get('author', '?')}",
            flush=True,
        )
        time.sleep(args.delay)

    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    with MANIFEST.open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")
    total_chars = sum(int(record["clean_chars"]) for record in records)
    print(
        f"wrote {len(records)} files, {total_chars:,} clean chars -> {CLEAN_DIR}",
        flush=True,
    )
    print(f"manifest -> {MANIFEST}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
