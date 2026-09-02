# Genesis Program Completion Standard

Genesis completion is calculated from leaf capabilities, not estimated from
file count, effort, or aspiration. Every leaf has nine weighted evidence gates:

| Gate | Weight |
|---|---:|
| Architecture and boundary design | 10% |
| Operational implementation | 25% |
| Unit tests | 15% |
| Integration tests | 15% |
| Repeatable benchmark | 10% |
| Security and abuse-boundary evidence | 10% |
| Recovery and failure testing | 5% |
| User/developer documentation | 5% |
| Named local-platform qualification | 5% |

A leaf is 100% only when every gate equals one. Parent percentages are the
arithmetic mean of their direct children; the Genesis percentage is the mean of
its top-level programs. Adding an unimplemented required child can lower the
parent percentage, which is intentional: discovering omitted scope must not make
the project appear closer to completion.

The authoritative inputs and generated report are:

- `registry/completion_components.tsv`
- `registry/completion_report.tsv`
- `tools/report_program_completion.ps1`

Current percentages describe repository evidence on the named local platform.
They do not transfer to other operating systems, hardware, models, devices,
firmware, networks, or production workloads without separate qualification.

Dependencies, drivers, firmware and model artifacts are installed only when a
registered leaf requires them and records license, provenance, version, rollback,
security, resource and verification evidence. Broad machine access is never used
as a substitute for an engineering reason or a recoverable change plan.
