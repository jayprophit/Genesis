# Stage 5 Memory and Cognition

Status: **in progress**. This increment implements the dependency foundation,
not the entire cognition family.

Implemented and tested: identity-scoped memory nodes, provenance digests,
explicit origin and sharing scope, associative feature/context activation,
positive/negative affect valence, confidence, separate trace accessibility,
declarative/procedural decay rates, dormancy, archival state, reinforcement,
consolidation, relearning, evidence-linked uncertain outcome prediction,
separated world/self/other belief categories, bounded workspace competition,
inhibition, numeric introspection, and bounded crash-safe snapshots with SHA-256
corruption detection, immutable versions, owner binding, path safety, atomic
commit and restore tests.

Feature and context indexes now limit associative scoring to relevant candidates.
This adapts the useful indexing lesson from the authorized Aetherius memory graph
while rejecting its wall-clock coupling, pseudo-random mock embeddings, replace-
on-conflict writes and unscoped global singleton. On the 10,000-node local
benchmark, 100 selective queries fell from approximately 0.73 seconds to 0.0099
seconds. This is a development-machine result, not a production guarantee.

Typed relationship traversal is bounded by depth and result count, filters by
relation, prevents cycles and accumulates path weights. World/self/other belief
records now carry claim keys, polarity and active/superseded/retracted state.
Contradictions are retained as evidence rather than silently overwritten;
supersession and retraction require explicit newer evidence. This adapts the
authorized Digital Twin belief-revision and contradiction requirements without
claiming that string claim keys provide general semantic understanding.

World, self and other-agent beliefs now carry an explicit evidence-source identity
and bounded source-quality value. Assessed confidence is calculated only for active
beliefs from original confidence, source quality and deterministic logical recency;
the original confidence remains intact for audit and alternative policies. Wall
clock time is not consulted. A zero freshness window, time reversal, inactive
belief or unknown belief produces no assessment rather than a fabricated score.

Belief models are now organism-bound and persist through schema-tagged, exact-value,
whole-record-checksummed immutable snapshots. Atomic temporary-file rename, size
limits and path-safe identifiers follow the existing persistence policy. Restore
replays beliefs through the public update and retraction rules, then verifies that
active, superseded and retracted states exactly match the stored model; derived
state is not trusted. Corruption, version conflict, unsafe path and identity tests
are included. On the local 10,000-belief benchmark, a 2,206,789-byte snapshot was
serialized and fully restored in approximately 0.711 seconds. This is development
evidence, not a production latency guarantee.

Completion accounting now separates persistent belief state from the broader
meaning of a world or self model. Evidence-qualified belief storage is complete
against its registered gates. Causal/temporal dynamics, counterfactual simulation,
capability calibration and autobiographical continuity remain separate incomplete
leaves; persistence alone does not justify calling the whole model complete.

The self model now includes an organism-bound capability and competency ledger.
Routes progress only through unavailable, declared, observed and qualified evidence
levels. Declaring a route does not prove that it works; recorded success and failure
outcomes establish a calibrated rate, and a separately supplied qualification
record is accepted only after policy thresholds are met. Repeated post-qualification
failures automatically demote a route, while explicit revocation makes it
unavailable. Qualification never authorizes execution: authenticated identity,
policy approval, safety approval and decision evidence must all pass independently.
The local benchmark declared and qualified 10,000 capabilities from 50,000 measured
outcomes in approximately 0.210 seconds. Persistence and migration of this ledger
remain open, so the capability-calibration leaf is intentionally not complete.

Causal world dynamics now separate immutable state observations, causal hypotheses,
time-bounded predictions and observed outcomes. Hypotheses retain prior confidence,
support and counterevidence counts; confirmation and refutation update a bounded
calibrated confidence without rewriting the originating evidence. Predictions are
explicitly pending, confirmed, refuted or expired and use logical time. Invalid
digests, duplicate IDs, missing hypotheses, reversed time, exhausted counters and
over-capacity inputs are rejected.

One-step counterfactual projection returns confidence-ranked consequences from
matching hypotheses under a strict result limit. Every returned item is explicitly
marked simulated so it cannot be mistaken for an observation, autobiographical
memory or fact. Multi-step intervention search now expands deterministic hypothesis
paths under explicit depth, branch, path, expansion and minimum-confidence limits.
It prevents path cycles, multiplies edge confidence without presenting it as proof,
reports depth, branch, path and resource truncation separately, flags unresolved branching
as a potential confounder, and can never authorize an action. Search results are
derived and never persisted as observations or memories; recovery tests demonstrate
that they can be regenerated from verified causal snapshots. The local benchmark
built 10,000 observations, hypotheses and predictions in approximately 0.665
seconds, resolved 10,000 outcomes in 0.047 seconds, and ran 1,000 bounded projections
in 0.310 seconds. These are development-machine measurements only.

Causal world state now persists in organism-bound schema-1 snapshots containing
observations, calibrated hypotheses, predictions, deadlines, outcome evidence and
resolution states. Exact numeric values and a whole-record SHA-256 checksum are
preserved under bounded field, item and file sizes. Immutable versions commit by
temporary-file rename. Restore rebuilds observations through their validator,
validates hypothesis calibration from prior/support/counterevidence, checks every
prediction against its hypothesis and rejects corruption, conflicts, unsafe paths,
identity mismatch and unsupported schemas. The 30,000-record local benchmark
produced a 7,783,499-byte snapshot and restored it in approximately 0.774 seconds.
Bounded counterfactual projections are regenerated from restored hypotheses rather
than serialized as facts.

The dedicated branching benchmark performed 1,000 bounded multi-step searches and
returned 15,944 simulated paths in approximately 1.133 seconds on the development
machine. This is a workload measurement, not a production latency guarantee or a
claim that the hypotheses establish causation.

Learning interference is now an explicit evidence-linked graph over registered
traces. The consolidation scheduler produces a deterministic, cost-bounded plan
from importance, uncertainty, accessibility, strongest interference, logical
disuse, and trace kind. It never mutates memory merely because a trace ranks
highly. Duplicate/reversed pairs, self-conflicts, unknown traces, invalid ratios,
budget overflow and logical-time reversal are rejected. The separate executor
now validates the complete plan and current trace-state digests, reserves bounded
runtime resources, atomically applies a reinforcement batch, verifies graph
invariants and appends a bounded SHA-256-linked execution record. Resource denial,
duplicate items and stale state leave memory unchanged. Retention evaluation is
owned by a separate component so execution evidence cannot conceal its remaining
persistence and migration gaps.

Longitudinal retention evaluation now records bounded, evidence-linked baseline
and follow-up observations for declarative, procedural, episodic and semantic
traces. Recall, response-latency quality and confidence remain separate inputs;
reports combine recall and latency using a documented deterministic score and
retain the raw observations. Reports identify improvement, stability or regression,
elapsed logical time, consolidation-record linkage and active interference. An
interference flag means association at the follow-up observation, not proof that
the interferer caused the regression. Feedback adjustments are bounded and must
be explicitly validated by the scheduler before they can change accessibility or
uncertainty. Observation records are sequence-ordered and SHA-256 chained.

The 10,000-trace local benchmark recorded 20,000 observations and generated
10,000 reports in approximately 0.214 seconds. Its 3,557,919-byte snapshot was
serialized, checksummed, restored, re-indexed and fully revalidated in approximately
0.546 seconds. These are machine-local development measurements, not production
latency guarantees.

Retention snapshots are now organism-bound, schema-tagged, size-bounded and
whole-record checksummed. Immutable versions commit through a temporary file and
atomic rename. Restore rebuilds observations through the public validator rather
than trusting serialized derived indexes or record-chain fields. Tests cover
roundtrip recovery, idempotent writes, conflicting immutable versions, unsafe
identifiers and corruption. Unsupported schemas fail explicitly; actual future
cross-schema migrations must be registered and tested when a second schema exists.

The prediction system retrieves multiple experiences and reports confidence; it
does not create categorical identity rules such as “person like this is bad.”
The workspace name is architectural terminology and is not evidence of
consciousness or subjective experience.

Still open in Stage 5: real perception adapters, language representation, affect
regulation, future
cross-schema migration implementations, fuzzing, and broader integration across
the organism event fabric.

`genesis2.txt` is preserved losslessly and indexed by all 88 Markdown heading
occurrences. Repeated passages remain traceable; referenced tools and repositories
are research data, not automatically approved dependencies.
