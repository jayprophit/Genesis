# Dataset registry

Every dataset used for training must have one record here before it is used.
A missing licence or hash is a CI failure. This file is a template: replace the
examples with real records as data is acquired.

```json
{
  "id": "ds-example-0001",
  "name": "Example corpus",
  "uri": "https://example.org/data",
  "licence": "CC-BY-4.0",
  "sha256": "0000000000000000000000000000000000000000000000000000000000000000",
  "retrieved_at": "2026-10-23",
  "tokens": 0,
  "languages": ["en"],
  "modality": "text",
  "allowed_use": "training",
  "consent_basis": "public licence",
  "contamination_notes": "",
  "redistribution": "allowed with attribution"
}
```

## Rules

1. Record the exact URI, retrieval date and SHA-256 of the bytes used.
2. Record the licence and whether derivative models may be released.
3. Do not mix a dataset with an unclear licence into a release candidate.
4. Private camera, microphone, screen or workspace data requires a separate,
   explicit training consent. It is never assumed from product use.
5. Keep a held-out split that the training job cannot read.
6. Re-check upstream terms before every public release.
