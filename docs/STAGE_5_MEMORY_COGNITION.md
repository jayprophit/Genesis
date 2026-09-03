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
