# Genesis Specification Completion Policy

`docs/specifications/source/genesis.txt` is the canonical design input. Its
1,451 indexed sections are permanent scope, not an automatic completion claim.

A requirement may advance only through evidence-backed states. Source text is
`DISCOVERED` or `SPECIFIED`; an interface without working behavior is
`SCAFFOLDED`; executable behavior requires implementation files and tests;
`BENCHMARKED` additionally requires a repeatable benchmark; platform or device
claims require observed and qualified evidence on the named route.

Requirements are never silently deleted, renumbered, or marked complete because
a neighbouring subsystem exists. Every implementation increment records its
source section, dependencies, files, tests, benchmark where applicable, version,
and verification date in `registry/requirements.tsv`.

“All of genesis.txt implemented” is the long-horizon completion target. It is
reached only when every applicable canonical section maps to verified requirement
evidence, all mandatory platform gates pass, and no required item remains merely
discovered, specified, scaffolded, unavailable, or adapter-required.
