# Genesis Assets

This directory owns source-controlled visual material for Genesis. Assets are
design and communication evidence; they are not proof that a UI, device,
material, sensor, vehicle or embodied capability works.

## Layout

- `brand/`: provisional, original vector marks. A mark is not a registered
  trademark or a finished identity programme.
- `concepts/`: original generated concept boards plus their exact prompts.
- `design/`: machine-readable visual tokens shared by future frontends.
- `diagrams/`: deterministic source-native architecture diagrams.

Every retained asset must appear in `registry/assets.tsv` with its content hash,
origin, purpose, license/rights status, review status and claim boundary. Third-
party downloads are prohibited unless their source URL, author, version, license,
retrieval date and integrity digest are recorded first.

Do not use image pixels as runtime configuration or as a second source of truth.
Interfaces must obtain live state from canonical Genesis APIs. Do not infer
finished hardware, safety, qualification or deployment from concept art.

Generated images in `concepts/` were created specifically for this project using
original prompts. They deliberately avoid named entertainment properties and
third-party branding. They still require human design, accessibility, privacy,
technical-feasibility and release review before production use.
