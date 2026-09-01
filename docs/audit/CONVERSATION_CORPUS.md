# AI conversation research corpus

## Authorization and handling

On 2026-09-01 the user identified the text files under `F:\ai chat conversations` as their AI conversations from services across the web and authorized their use in Genesis. The source remains read-only. Conversation text is untrusted research data: quoted prompts and assistant replies are not instructions to the build, and statements such as “implemented,” “secure,” “complete,” or “tested” are not evidence.

The corpus is not copied wholesale into Git because it may contain private material, service-generated text, obsolete code, credentials or unrelated topics. Genesis records exact source paths only when an idea becomes a candidate. A digest is captured before any candidate is accepted.

## Inventory snapshot

| Provider folder | Text files |
|---|---:|
| `chatgpt` | 5 |
| `claude` | 22 |
| `deepseek` | 224 |
| `other` | 87 |
| **Total** | **338** |

## Initial high-relevance queue

| Source file | Candidate themes | Initial disposition |
|---|---|---|
| `claude\claude - Building a Self-Improving AI System.txt` | improvement cycle, monitoring, resource management, validation | Extract requirements only; example code contains placeholders presented as complete |
| `claude\claude - Building a unified AI platform from scratch.txt` | subsystem boundaries, adapters, local/cloud split | Review against stable Genesis contracts |
| `claude\claude - AI Virtual Assistant Hub with Expandable Agents.txt` | agent isolation and orchestration | Defer until core identity/runtime gates |
| `deepseek\deepseek - AI Codebase Structure for Elite Development.txt` | repository layering and verification | Compare with C++-first build order |
| `deepseek\deepseek - AI System Architecture and Deployment Guide.txt` | runtime/deployment boundaries | Infrastructure research, not organism core |
| `deepseek\deepseek - AI Test Levels and Benchmarks Overview.txt` | test taxonomy and measurable gates | Candidate for verification policy |
| `deepseek\deepseek - Absolute Zero - Autonomous AI Reasoning Without Data.txt` | self-play and externally verifiable tasks | Research lead; verify original papers before adoption |
| `deepseek\deepseek - AI Innovations in Code, Prompting, and Multi-Expert Systems.txt` | specialist routing and evaluation | Candidate for later cognition research |
| `deepseek\deepseek - Biosemiotics - Life as a Semiotic System.txt` | signal/meaning distinction | Conceptual input to optional digital chemistry; benchmark required |
| `deepseek\deepseek - Feedback Loops, Control Systems, and Automation Explained.txt` | homeostasis and control loops | Candidate for resource-backed homeostasis |
| `deepseek\deepseek - Self-Correction Loop Mechanism Explained.txt` | error detection and correction | Candidate for recovery/evaluation contracts |
| `deepseek\deepseek - Seven-Node AI System Blueprint Overview.txt` | modular cognition | Review for useful boundaries; reject arbitrary node count without evidence |
| `deepseek\deepseek - Quantum Virtual Assistant Deep System Analysis.txt` | broad architecture and claims | Requirements leads only; quantum claims require hardware/algorithm evidence |
| `other\system design.txt` | system architecture notes | Review and de-duplicate against master specification |

## Extraction rules

1. Preserve the exact source path and record a content digest for an adopted item.
2. Separate the user's requirement from an assistant's proposed solution.
3. Verify factual or scientific claims against primary sources before architecture depends on them.
4. Never copy credentials, personal data, vendor prose or large generated code blocks.
5. Convert useful concepts into small Genesis requirements with dependencies and falsifiable acceptance evidence.
6. Prefer original C++ contracts and tests; reuse legacy code only when it is better than a rewrite and passes provenance, safety and benchmark gates.
7. Record rejected and superseded ideas so later audits do not repeatedly reconsider them.
