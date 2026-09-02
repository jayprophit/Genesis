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

Learning interference is now an explicit evidence-linked graph over registered
traces. The consolidation scheduler produces a deterministic, cost-bounded plan
from importance, uncertainty, accessibility, strongest interference, logical
disuse, and trace kind. It never mutates memory merely because a trace ranks
highly. Duplicate/reversed pairs, self-conflicts, unknown traces, invalid ratios,
budget overflow and logical-time reversal are rejected. The separate executor
now validates the complete plan and current trace-state digests, reserves bounded
runtime resources, atomically applies a reinforcement batch, verifies graph
invariants and appends a bounded SHA-256-linked execution record. Resource denial,
duplicate items and stale state leave memory unchanged. Retention and interference
outcome measurement remain separate work and are tracked as their own completion
leaf so execution evidence cannot conceal that gap.

The prediction system retrieves multiple experiences and reports confidence; it
does not create categorical identity rules such as “person like this is bad.”
The workspace name is architectural terminology and is not evidence of
consciousness or subjective experience.

Still open in Stage 5: persistent belief-model storage, evidence-source quality,
real perception adapters, language representation, affect regulation, retention
evaluation, fuzzing, and broader integration across the organism event fabric.

`genesis2.txt` is preserved losslessly and indexed by all 88 Markdown heading
occurrences. Repeated passages remain traceable; referenced tools and repositories
are research data, not automatically approved dependencies.
