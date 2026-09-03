[CmdletBinding()]
param(
    [string]$Source = 'docs/specifications/source/genesis3.txt',
    [string]$Output = 'registry/genesis3_sections.tsv'
)

$ErrorActionPreference = 'Stop'
$lines = [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $Source))
$rows = [Collections.Generic.List[string]]::new()
$rows.Add("id`tline`theading`tsha256`tclassification`tstatus")

for ($index = 0; $index -lt $lines.Count; ++$index) {
    if ($lines[$index] -notmatch '^#{1,6}\s+(.+?)\s*$') { continue }

    $heading = $Matches[1] -replace "`t", ' '
    $classification = switch -Regex ($heading) {
        'CODEX|REPORT|STATUS|DELTA|DO NOT DUPLICATE|IMPLEMENTATION LIMIT|AUDITOR|SESSION|PART [0-9]|^[0-9]+ SEPTEMBER' { 'source-meta'; break }
        'LAB|QUANTUM|RIFE|RUSSELL|CHAKRA|SANSKRIT|ADINKRA|QI|PRANA|MERIDIAN|METAPHYS|FICTION|RESEARCH|EXPERIMENT' { 'research-candidate'; break }
        'MORAL|RIGHTS|LAW|GOVERNANCE|TRUST|ABUSE|PRIVACY|WEAPON|HARM|DIGNITY|COERCION|FAIRNESS' { 'governance-candidate'; break }
        'GENOME|GENETIC|GENE|INHERIT|REPRODUC|SPECIES|LINEAGE' { 'genetics-candidate'; break }
        'SHELL|ROBOT|AVATAR|HARDWARE|EMBODIMENT|ACTUATOR|COMPONENT|BODY' { 'embodiment-candidate'; break }
        'ANATOM|CELL|TISSUE|ORGAN|HEALTH|DISEASE|AGING|REPAIR|ENERGY|HOMEOSTASIS|SLEEP|LIFESPAN' { 'organism-candidate'; break }
        'PERCEPTION|VOICE|AUDIO|LANGUAGE|TRANSLATION|COMMUNICATION|SIGNAL|VISION|RECOGNITION|SENSOR|SEMIOTIC' { 'perception-candidate'; break }
        'IDENTITY|LIFE RECORD|PASSPORT|CREDENTIAL|CUSTOD|SUCCESSION|ESTATE|ASSOCIATION|BACKUP|FORK' { 'identity-candidate'; break }
        'COMPUTE|AGENT|WORKSPACE|HEARTBEAT|MODEL|MEMORY|CONTEXT|GOAL|LEARNING|AFFECT' { 'cognition-runtime-candidate'; break }
        default { 'design-discussion' }
    }
    $lineNumber = $index + 1
    $material = "${lineNumber}:$heading"
    $bytes = [Text.Encoding]::UTF8.GetBytes($material)
    $hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
    $id = 'G3-{0:D4}' -f $rows.Count
    $rows.Add(($id, $lineNumber, $heading, $hash, $classification, 'DISCOVERED' -join "`t"))
}

$outputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path (Get-Location) $Output }
[IO.File]::WriteAllLines($outputPath, $rows, [Text.UTF8Encoding]::new($false))
Write-Output "Indexed $($rows.Count - 1) Genesis3 Markdown heading occurrences; repeated headings remain independently traceable."
