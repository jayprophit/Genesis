[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$Output,
    [Parameter(Mandatory)][ValidatePattern('^[A-Z][A-Z0-9]*$')][string]$Prefix
)

$ErrorActionPreference = 'Stop'
$lines = [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Source))
$rows = [Collections.Generic.List[string]]::new()
$rows.Add("id`tline`theading`tsha256`tclassification`tstatus")

for ($index = 0; $index -lt $lines.Count; ++$index) {
    if ($lines[$index] -notmatch '^#{1,6}\s+(.+?)\s*$') { continue }

    $heading = $Matches[1] -replace "`t", ' '
    $classification = switch -Regex ($heading) {
        'CODEX|REPORT|STATUS|DELTA|DO NOT (RE-ADD|DUPLICATE)|IMPLEMENTATION LIMIT|AUDITOR|SESSION|CONSOLIDATION PASS|PART [0-9]|^[0-9]+ SEPTEMBER|NEXT ACTION|HOW TO USE' { 'source-meta'; break }
        'LAB|QUANTUM|RIFE|RUSSELL|CHAKRA|SANSKRIT|ADINKRA|QI|PRANA|MERIDIAN|METAPHYS|FICTION|RESEARCH|EXPERIMENT|FRACTAL|HOLONOMIC' { 'research-candidate'; break }
        'MORAL|ETHIC|RIGHTS|LAW|GOVERNANCE|TRUST|ABUSE|PRIVACY|WEAPON|HARM|DIGNITY|COERCION|FAIRNESS|CONSTITUTION|COMPLIANCE' { 'governance-candidate'; break }
        'GENOME|GENETIC|GENE|DNA|RNA|INHERIT|REPRODUC|SPECIES|LINEAGE|EVOLUTION' { 'genetics-candidate'; break }
        'SHELL|ROBOT|AVATAR|HARDWARE|EMBODIMENT|ACTUATOR|COMPONENT|BODY|DEVICE|ACCESSIBILITY|UI' { 'embodiment-candidate'; break }
        'ANATOM|CELL|TISSUE|ORGAN|HEALTH|DISEASE|AGING|REPAIR|ENERGY|HOMEOSTASIS|SLEEP|LIFESPAN|LIFE COMMUNICATION' { 'organism-candidate'; break }
        'PERCEPTION|VOICE|AUDIO|LANGUAGE|TRANSLATION|COMMUNICATION|SIGNAL|VISION|RECOGNITION|SENSOR|SEMIOTIC|MULTI-MODAL' { 'perception-candidate'; break }
        'IDENTITY|LIFE RECORD|PASSPORT|CREDENTIAL|CUSTOD|SUCCESSION|ESTATE|ASSOCIATION|BACKUP|FORK|OWNER|TENANT|AUTHENTICATION' { 'identity-candidate'; break }
        'COMPUTE|AGENT|WORKSPACE|HEARTBEAT|MODEL|MEMORY|CONTEXT|GOAL|LEARNING|AFFECT|REASONING|KNOWLEDGE|INGESTION|DEDUPLICATION' { 'cognition-runtime-candidate'; break }
        default { 'design-discussion' }
    }
    $lineNumber = $index + 1
    $material = "${lineNumber}:$heading"
    $bytes = [Text.Encoding]::UTF8.GetBytes($material)
    $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
    $id = '{0}-{1:D4}' -f $Prefix, $rows.Count
    $rows.Add(($id, $lineNumber, $heading, $hash, $classification, 'DISCOVERED' -join "`t"))
}

$outputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path (Get-Location) $Output }
[IO.File]::WriteAllLines($outputPath, $rows, [Text.UTF8Encoding]::new($false))
Write-Output "Indexed $($rows.Count - 1) $Prefix Markdown heading occurrences; repeated headings remain independently traceable."
