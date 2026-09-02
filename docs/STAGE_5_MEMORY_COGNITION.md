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

The prediction system retrieves multiple experiences and reports confidence; it
does not create categorical identity rules such as “person like this is bad.”
The workspace name is architectural terminology and is not evidence of
consciousness or subjective experience.

Still open in Stage 5: contradiction resolution, real perception adapters,
language representation, learning-policy
governance, affect regulation, benchmarks, fuzzing, and integration tests across
the organism event fabric.

`genesis2.txt` is preserved losslessly and indexed by all 88 Markdown heading
occurrences. Repeated passages remain traceable; referenced tools and repositories
are research data, not automatically approved dependencies.
