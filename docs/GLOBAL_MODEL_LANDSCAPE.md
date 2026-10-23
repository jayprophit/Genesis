# Global model landscape — study notes for Genesis

**Purpose.** Genesis must be designed with awareness of the whole world's model ecosystem, not only US and European labs. This document records the families, mechanisms and product patterns worth studying, and what Genesis should adopt, defer or reject. It is **not** a source of weights. Every model named here is a reference; none is a dependency.

**Rule.** We study published papers, model cards, documentation and observed behaviour. We do not copy weights, tokenizers, embedding tables or proprietary internals. A closed model can teach us product behaviour; it cannot give us parameters.

---

## 1. How to read this landscape

Four different things are often mixed together:

1. **Research mechanism** — attention variant, optimiser, RL objective, data curriculum.
2. **Model family** — a lineage with sizes and versions.
3. **Product behaviour** — tool schemas, refusal style, latency, UX, agent loop.
4. **Deployment posture** — open weights, open source, API-only, on-device, sovereign.

Genesis should adopt mechanisms deliberately, learn product behaviour by testing, and choose its own deployment posture: **local-first, open model card, proprietary brand, optional cloud**.

---

## 2. United States

| Family | Posture | What to study | What Genesis takes |
|---|---|---|---|
| OpenAI GPT | closed, API/product | tool schema quality, structured outputs, reasoning effort controls, multimodal UX, safety layers | schema discipline; product expectations; not weights |
| Anthropic Claude | closed, API/product | long-context behaviour, constitutional/RLHF alignment, computer-use tool loop, artifact UX | tool-loop safety; honest uncertainty; not weights |
| Google Gemini | closed + some open (Gemma) | native multimodality, long context, on-device (Gemma) distillation, TPU training | early fusion; small-model distillation |
| Meta Llama | open weights | open serving ecosystem, GQA adoption, instruction tuning recipe, community fine-tunes | architecture defaults; ecosystem compatibility |
| xAI Grok | partly open | large-scale training infrastructure, real-time data integration | infrastructure lessons only |
| Microsoft Phi | open weights | small-model data quality, textbook-style synthetic curricula | data quality over raw size for small models |
| NVIDIA Nemotron | open weights + tools | synthetic data generation, RL environments, GPU-optimised inference | data generation and evaluation environments |
| Ai2 OLMo / Molmo | fully open (data, code, eval) | reproducible pretraining, fully open multimodal recipe | **the transparency template** |
| Apple / Google on-device | closed | quantization, distillation, latency and privacy budgets | local-first runtime targets |
| Liquid AI | open/closed | non-transformer sequence models | long-context experiment only |

**Lesson:** the US leads on product integration and scale, and OLMo/Molmo lead on reproducibility. Genesis should copy the *discipline* of OLMo (data and eval transparency) and the *product discipline* of the frontier APIs, not their scale claims.

---

## 3. China

China is the most important region for Genesis's architecture choices because it publishes strong mechanisms and open weights at a rapid cadence.

### 3.1 DeepSeek

- **Mechanisms:** Multi-head Latent Attention (MLA) compresses KV cache; DeepSeekMoE uses fine-grained experts plus shared experts; multi-token prediction (MTP); FP8 mixed-precision training; GRPO for reasoning RL; strong code and math data.
- **Why it matters:** it shows how to get long-context quality at a fraction of the KV memory, and how to train MoE efficiently.
- **Genesis:** GQA now; **MLA evaluation at v2**; MTP auxiliary head at v2; GRPO for tool use at v3. Do not attempt FP8 on a GTX 1050 Ti.

### 3.2 Alibaba Qwen

- **Mechanisms:** broad family from 0.5B to large MoE; strong multilingual (especially Chinese and Asian languages); Qwen-VL early visual fusion; tool-use formats; aggressive open-weight releases; small models that run locally.
- **Why it matters:** the best reference for **breadth of sizes**, multilingual data and small-model usability.
- **Genesis:** the size ladder and multilingual tokenizer coverage; Qwen-VL's early-fusion pattern; the discipline of releasing usable small models.

### 3.3 Moonshot Kimi

- **Mechanisms:** long-context training, agentic RL, tool use, MuonClip-style optimisation on very large runs, hybrid attention in newer families.
- **Why it matters:** agentic behaviour and long context are closer to Genesis's product than raw trivia.
- **Genesis:** tool-use SFT and RL; long-context curriculum; agent evaluation.

### 3.4 Zhipu GLM

- **Mechanisms:** agentic/tool RL, bilingual Chinese/English, open weights, strong instruction following.
- **Genesis:** tool-call curriculum and bilingual evaluation.

### 3.5 MiniMax

- **Mechanisms:** hybrid lightning/full attention, MoE, long context, efficient inference.
- **Genesis:** hybrid attention experiment at v2/v3; not v1.

### 3.6 ByteDance Seed / Doubao

- **Mechanisms:** large-scale multimodal training, UI/agent models (UI-TARS-style), video generation, product integration.
- **Genesis:** UI-grounding data and computer-use evaluation; video is out of scope for v1.

### 3.7 Tencent Hunyuan

- **Mechanisms:** open multimodal and 3D/video models; MoE; enterprise deployment.
- **Genesis:** multimodal data and 3D/embodiment references, not model weights.

### 3.8 Baidu ERNIE

- **Posture:** primarily closed/API.
- **Study:** Chinese-language product behaviour, search integration, safety policy.

### 3.9 Others

- **01.AI Yi:** bilingual small/medium models; data quality.
- **InternLM / Shanghai AI Lab:** open training frameworks and multilingual corpora.
- **OpenBMB MiniCPM:** extreme small-model efficiency, on-device multimodal; important for Genesis's 4 GB target.
- **StepFun Step:** multimodal and efficient architectures.
- **Xiaomi MiMo:** small reasoning models; on-device.
- **Huawei Pangu:** closed, industrial/enterprise; domain deployment lessons.
- **Ant Ling, Meituan LongCat:** MoE and efficient training experiments.

**China lesson:** the open Chinese ecosystem is the best available teacher of **MoE, MLA, low-bit training, long context and agentic RL**. Genesis should adopt those mechanisms on a schedule matched to its hardware, and should build genuine Chinese/Asian language capability rather than treating it as an afterthought.

---

## 4. Europe

| Family / lab | Posture | Study | Genesis |
|---|---|---|---|
| Mistral (FR) | open weights + commercial | sparse MoE, edge models, efficient inference, European-language data | efficiency and EU language coverage |
| Aleph Alpha (DE) | closed/enterprise | sovereign deployment, auditability, European public-sector requirements | governance and deployment model |
| DeepMind (UK) | closed | multimodal research, RL, evaluation science | research papers; not weights |
| Stability AI (UK) | open/closed | generative media, open model release practice | media tooling only |
| Hugging Face (FR/US) | open ecosystem | dataset/model cards, evaluation tooling, community infrastructure | provenance and model-card practice |
| ETH/EPFL | research | efficient training, systems, multilingual | research collaboration references |

**Europe lesson:** regulation and documentation are product features. The EU AI Act favours systems that can explain data, risk and evaluation. Genesis's provenance-first design is aligned with that.

---

## 5. Japan

- **Sakana AI:** evolutionary model merging, small-model composition, Japanese research; study the evaluation discipline, not merge as a base strategy.
- **Preferred Networks PLaMo:** Japanese-language LLM, enterprise focus.
- **Rakuten AI:** Japanese/English models trained for commerce.
- **SB Intuitions:** Japanese LLM.
- **NTT tsuzumi:** compact Japanese enterprise model.

**Genesis:** Japanese tokenizer coverage, Japanese evaluation set, and respect for local data sovereignty.

---

## 6. Korea

- **Naver HyperCLOVA X:** Korean-centric data and evaluation, enterprise/cloud integration.
- **LG EXAONE:** bilingual Korean/English, open weights for some sizes.
- **Upstage Solar:** compact models, Korean/English.
- **Kakao Kanana:** Korean consumer models.

**Genesis:** Korean corpus and a Korean benchmark track. Do not assume a Chinese/English tokenizer is sufficient.

---

## 7. India

- **Sarvam:** Indic-language models and voice; strong code-mixed data.
- **Krutrim:** Indian-language and compute sovereignty.
- **AI4Bharat:** IndicBERT, IndicTrans, corpora and benchmarks.
- **IIT/IISc labs:** speech, translation, low-resource NLP.

**Genesis:** Indic script coverage (Devanagari, Bengali, Tamil, Telugu, etc.), code-mixed Hinglish data, and a translation/QA evaluation. Tokenizer fertility matters more than parameter count here.

---

## 8. Middle East

- **TII Falcon (UAE):** open weights, long-context and data-quality research (RefinedWeb), Arabic/English.
- **G42 Jais (UAE):** Arabic-centric models.
- **SDAIA ALLaM (Saudi):** Arabic/English, sovereign deployment.

**Genesis:** Arabic script and right-to-left handling, Arabic evaluation, and data-licence review.

---

## 9. South-East Asia

- **SEA-LION (Singapore):** multilingual SEA models and evaluation.
- **Sea AI Lab:** efficient training and research.
- **VinAI PhoGPT (Vietnam):** Vietnamese model.
- **Sahabat-AI (Indonesia):** Indonesian-language community model.
- **Mahakarya/other regional efforts:** local-language corpora.

**Genesis:** SEA language track if the product targets the region; otherwise document the gap honestly.

---

## 10. Other regions

- **Israel — AI21 Jamba:** Mamba-Transformer hybrid, long context. Study hybrid attention; do not assume it beats a well-tuned transformer at small scale.
- **Russia — YandexGPT, GigaChat:** sovereign Russian-language models; deployment and language coverage.
- **Taiwan — TAIDE:** Traditional Chinese model and evaluation.
- **Africa — Masakhane, Lelapa InkubaLM:** low-resource African languages, community data governance.
- **Latin America — LatinX/regional consortia:** Spanish/Portuguese coverage and evaluation.

**Lesson:** "multilingual" is not a checkbox. Tokenizer fertility, script handling, cultural evaluation and data consent differ by region. Genesis should publish a language coverage table and refuse to claim languages it has not evaluated.

---

## 11. Closed-source products — what can and cannot be learned

| Product class | Can study | Cannot obtain | Genesis response |
|---|---|---|---|
| GPT / Claude / Gemini | API behaviour, tool schemas, latency, refusal patterns, multimodal UX | weights, training data, internals | benchmark against them; do not imitate identity or claim parity |
| Grok | product integration, real-time data | weights (where closed) | study UX |
| ERNIE / Doubao / Hunyuan / Pangu | Chinese product behaviour, enterprise integration | weights (where closed) | study domain deployment |
| Command A / Jamba | enterprise RAG, hybrid architecture papers | weights (where closed) | study retrieval and hybrid attention |
| HyperCLOVA X / EXAONE / PLaMo | local-language product quality | weights (where closed) | build local-language evals |
| Cursor / Copilot / Devin / Manus | agent UX, tool loops, sandboxing, approvals | model and orchestration internals | study workflow and safety, not code |

**Never:** reverse-engineer a closed model to extract weights, bypass rate limits, or present a clone as Genesis.

---

## 12. Mechanism adoption plan for Genesis

| Mechanism | Source families | v1 (now) | v2 (1–3B) | v3 (7B+) |
|---|---|---|---|---|
| Byte-level BPE, multilingual | Qwen, Llama, Gemma | **yes** | larger vocab | 128k vocab |
| GQA + RoPE + SwiGLU + RMSNorm | Llama, Qwen, Mistral | **yes** | yes | yes |
| Early vision fusion | Qwen-VL, Molmo, Gemini | **yes** | stronger encoder | video |
| Native tool tokens | Kimi, Qwen, Claude | **yes** | constrained decode | RL tool use |
| MLA | DeepSeek | no | **evaluate** | yes |
| Fine-grained MoE + shared experts | DeepSeek, Qwen, MiniMax | no | **evaluate** | yes |
| Multi-token prediction | DeepSeek, Meta | no | auxiliary head | yes |
| GRPO / verifiable RL | DeepSeek, Kimi | no | tool tasks | broad |
| FP8 training | DeepSeek, Qwen | no | no | H100+ |
| Hybrid linear attention | MiniMax, Jamba, Qwen | no | experiment | evaluate |
| On-device quantization | MiniCPM, Gemma, Phi | int8 later | int4 | int4/QAT |
| Fully open data/eval | OLMo, Pythia | **policy** | yes | yes |
| Multilingual regional data | SEA-LION, Sarvam, HyperCLOVA, Falcon, ALLaM | English + Chinese | + Japanese/Korean/Arabic/Indic | broad |

---

## 13. What Genesis should not do

1. **Do not chase frontier scale.** A 1B model trained on excellent data and tools is more useful than a badly trained 7B.
2. **Do not copy a tokenizer** to save a week; it poisons the independence claim and often fits other languages poorly.
3. **Do not distil a closed API** into the base model and call it independent. Distillation from *our own* larger Genesis model is allowed and should be labelled.
4. **Do not claim a language or modality** that has no evaluation.
5. **Do not use "open source"** if weights, data or licence do not meet the OSI/Open Source AI definition. Say "open weights" precisely.
6. **Do not import a geopolitical data source** without checking licence, consent and export rules.
7. **Do not benchmark only on English.** Tokenizer fertility and task accuracy differ sharply by script.

---

## 14. Sources to track continuously

- Papers: arXiv cs.CL / cs.LG / cs.CV; NeurIPS/ICML/ICLR/ACL/EMNLP/CVPR.
- Model cards and technical reports from the families above.
- Open evaluation: LMSYS Arena (style/ preference), HELM, Open LLM Leaderboard, MMLU-Pro, GPQA, IFEval, BFCL (tool calling), SWE-bench, MMMU, DocVQA, MTEB, SEA-HELM, IndicBench, ArabicBench.
- Reproducibility: OLMo, Pythia, BLOOM, DataComp, OBELICS, The Stack.
- Safety: MLCommons safety, JailbreakBench, HarmBench.
- Regional: SEA-LION, AI4Bharat, IndicBench, Korean/Japanese/Arabic national benchmarks.

Treat leaderboard numbers as **claims with conditions**, not as properties of a model. Re-run what matters on Genesis hardware and data.

---

## 15. Bottom line

The world's model ecosystem offers Genesis a clear menu:

- **from the US:** product discipline, tool UX, reproducibility (OLMo), small-model data quality (Phi);
- **from China:** MLA, MoE, MTP, GRPO, FP8, long context, multilingual breadth, open-weight cadence;
- **from Europe:** efficiency, documentation, regulation-ready governance;
- **from Japan/Korea/India/Middle East/SEA:** language coverage, tokenizer fertility, regional evaluation and sovereignty;
- **from closed products:** behaviour to benchmark, never weights to copy.

Genesis-1 should be a **dense, multilingual, tool-native, image-aware model** that runs locally and tells the truth about what it can do. Genesis-2 can add MoE, MLA and RL. Genesis-3 can compete at 7B scale. Trying to start at Genesis-3 is how the project fails.
