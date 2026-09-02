# F-Drive Text and Markdown Corpus Audit

## Scope

The user-supplied pointer `C:\Users\jpowe\Documents\fdrive data.txt`
identified four conversation-export roots and `F:\AI Digital Twin`. The audit
read `.txt` and `.md` files without modifying the F drive. Generated dependency,
cache, build, Git, distribution, target and virtual-environment directories were
excluded so package documentation was not mistaken for user research.

The reproducible inventory contains 442 files and 20,676,446 bytes. It extracted
519 unique HTTP(S) strings. Two exact duplicate-content groups were found and are
retained by path and hash rather than copied twice. One 620-byte ChromeOS README
could not be read because Windows reported a cyclic-redundancy-check error; it is
recorded as `UNREADABLE_CRC` and was not silently treated as empty evidence.

## Topic distribution

Keyword routing found substantial overlap because many files are long AI
conversations rather than single-purpose specifications:

| Topic | Matching files |
|---|---:|
| Networking and distributed systems | 340 |
| Provenance, sources or licensing | 311 |
| Multimodal, sensing or embodiment | 294 |
| Code and implementation | 293 |
| Security | 260 |
| Learning and training | 255 |
| Development and teaching | 232 |
| Cognition | 219 |
| Memory | 214 |
| Descendants or evolution | 114 |

These counts indicate where to investigate; they are not quality scores or proof
that a proposed capability works.

## Link analysis

Of 519 unique strings, many are examples rather than external evidence: 50 use
`localhost`, 33 contain URI templates or invalid placeholders, and others point
to demo, placeholder, login, image badge or invented repository addresses. They
remain `UNVERIFIED` in the link registry and must not become dependencies.

The initial high-utility verification set found:

- `llama.cpp`: strong C/C++ local inference adapter candidate with broad CPU/GPU
  backends. Keep it outside organism identity, memory and cognition.
- DeepSeek-R1 and Qwen3: model-evaluation candidates. Register exact artifacts,
  hashes, licenses, context limits and local measurements; never equate a model
  with Genesis itself.
- Pipecat: useful reference for streaming voice/multimodal pipelines and
  specialist handoffs, but it is a Python framework/sidecar rather than the
  Genesis event fabric.
- Piper: the linked repository is archived and points to a GPL successor. The
  old link is unsuitable as a fresh dependency without reviewing the successor.
- OpenVoice: potentially useful for consent-gated voice output research. Voice
  ownership, speaker consent and dataset/model provenance remain mandatory.
- FFmpeg: useful multimedia adapter surface, but build-time LGPL/GPL choices and
  codec availability must be recorded precisely.
- Text2Robot: research input for simulation-first embodiment and manufacturing
  constraints, not permission for autonomous design, fabrication or actuation.

## Accepted abstractions

1. A versioned local-model adapter ABI with artifact hash, license, backend,
   measured memory, latency and context limits.
2. Streaming media frames as provenance-bearing observations on the bounded
   event fabric, never direct cognition or direct action.
3. Voice and avatar routes with explicit consent, identity, source-model and
   generated-media records.
4. Multimedia capability manifests that record codecs, devices and actual build
   flags rather than assuming all FFmpeg functionality exists.
5. Specialist handoff as an authenticated intent/result protocol with private
   memory isolation and bounded shared-domain exchange.
6. Simulation-before-action and qualification gates for every physical route.
7. Research/model outputs remain observations; they cannot directly mutate
   identity, memory, policy, hardware or descendant state.

## Rejected or deferred material

- Placeholder and localhost links as external evidence.
- Invented `your-org`, `yourusername`, demo and URI-template repositories.
- Archived Piper as a new dependency.
- Quantum, transcendence or speculative hardware claims without reproducible
  primary evidence and local qualification.
- Wholesale copying of conversation-generated code or third-party projects.
- Direct voice cloning without consent and provenance gates.
- Direct physical robot generation or actuation from text.

The complete file and link inventories are `registry/fdrive_research_files.tsv`
and `registry/fdrive_links.tsv`; the independently checked priority subset is
`registry/fdrive_verified_links.tsv`.
