param(
    [Parameter(Mandatory = $false)]
    [string]$Source = "docs/specifications/source/genesis.txt",

    [Parameter(Mandatory = $false)]
    [string]$Output = "registry/canonical_sections.tsv"
)

$ErrorActionPreference = "Stop"

function Normalize-Field([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "-"
    }
    return (($Value -replace "`t", " " -replace "`r?`n", " " -replace "\s+", " ").Trim())
}

function Get-Domain([string]$Title) {
    $value = $Title.ToUpperInvariant()
    if ($value -match 'GENOME|GENETIC|DNA|RNA|HELIX|REPRODUC|BIRTH|LINEAGE|ANCEST|CHILD|FAMILY|TWIN|SIBLING') { return 'DOMAIN-GENETICS' }
    if ($value -match 'IDENTITY|CONTINUITY|SELF BOUNDARY|CLONE|FORK|BACKUP') { return 'DOMAIN-IDENTITY' }
    if ($value -match 'CELL|TISSUE|ORGAN|ANATOMY|APOPTOSIS|DIFFERENTIATION|MORPHOGEN') { return 'DOMAIN-ORGANISM' }
    if ($value -match 'MEMORY|RETRIEV|RAG|KNOWLEDGE GRAPH|FORGET|CONSOLIDAT|CONTEXT') { return 'DOMAIN-MEMORY' }
    if ($value -match 'LEARN|CURRICUL|PLAY|IMITATION|DEVELOP|MATUR|TEACH|PARENTING') { return 'DOMAIN-DEVELOPMENT' }
    if ($value -match 'COGNIT|BRAIN|NEUR|SYNAP|ATTENTION|WORKSPACE|THOUGHT|REASON|IMAGIN|BELIEF|EPISTEM|CURIOS|CREATIV|INTELLIGENCE') { return 'DOMAIN-COGNITION' }
    if ($value -match 'AFFECT|EMOTION|SOCIAL|THEORY OF MIND|VALUE|CONSEQUENCE|GOAL|PLANNING') { return 'DOMAIN-AFFECT-SOCIAL' }
    if ($value -match 'VISION|AUDIO|AUDITION|SENSOR|PERCEPT|RECOGNITION|MULTIMODAL|SPEECH|LANGUAGE|VOICE|TOUCH|THERMAL|OLFACT|TASTE') { return 'DOMAIN-PERCEPTION' }
    if ($value -match 'BODY|AVATAR|ROBOT|MOTOR|PROPRIO|WEARABLE|DEVICE|GLASSES|WATCH|RING|CLOTHING|ACTUATOR|HARDWARE') { return 'DOMAIN-EMBODIMENT' }
    if ($value -match 'SECUR|CRYPTO|AUTH|PERMISSION|TRUST|PRIVACY|THREAT|ENCRYPT|KEY|E-STOP|ACCESS CONTROL') { return 'DOMAIN-SECURITY' }
    if ($value -match 'EVENT|RUNTIME|SCHEDUL|TIME|CLOCK|STATE|TRANSACTION|RESOURCE|ENERGY|HOMEOSTASIS|METABOL|TELEMETRY|OBSERV|LOG|SERVICE|JOB|QUEUE') { return 'DOMAIN-RUNTIME' }
    if ($value -match 'STORAGE|DATABASE|FILESYSTEM|SNAPSHOT|RECOVERY|BACKUP|MIGRATION') { return 'DOMAIN-STORAGE' }
    if ($value -match 'NETWORK|CLOUD|DISTRIBUT|HIVE|PROTOCOL|API|SESSION|SYNC|REMOTE|SERVER|OFFLINE|INTERNET') { return 'DOMAIN-NETWORK' }
    if ($value -match 'TOOL|SKILL|PLUGIN|AGENT|AUTOMATION|WORKFLOW|BROWSER|TERMINAL') { return 'DOMAIN-TOOLS' }
    if ($value -match 'BUILD|TEST|BENCH|SCORE|EVIDENCE|PROVEN|REQUIREMENT|REGISTRY|DEPENDENCY|VERSION|RELEASE|INSTALL|PACKAGE|LICENSE|MANIFEST|COMPATIBILITY') { return 'DOMAIN-ENGINEERING' }
    if ($value -match 'RESEARCH|SCIENCE|PATENT|FICTION|SPECULATION|UAP|TECHNOLOGY|PRIOR ART|EXPERIMENT|SIMULATION|ANOMALY') { return 'DOMAIN-RESEARCH' }
    if ($value -match 'MODEL|LLM|AI ROUTER|NARROW AI|PRECISION|TOKEN|BYTE|NEUROMORPH|PHOTONIC|QUANTUM|TERNARY') { return 'DOMAIN-COMPUTE' }
    return 'DOMAIN-GOVERNANCE'
}

$resolvedSource = (Resolve-Path -LiteralPath $Source).Path
$lines = Get-Content -LiteralPath $resolvedSource
$marker = '# START OF GENESIS CANONICAL SPECIFICATION'
$markerIndex = [Array]::IndexOf($lines, $marker)
if ($markerIndex -lt 0) {
    throw "Canonical specification marker not found in $resolvedSource"
}

$sections = [System.Collections.Generic.List[object]]::new()
for ($index = $markerIndex + 1; $index -lt $lines.Count; $index++) {
    if ($lines[$index] -match '^#{1,6}\s+(\d{1,4})\.\s+(.+?)\s*$') {
        $sections.Add([pscustomobject]@{
            Number = [int]$Matches[1]
            Title = $Matches[2].Trim()
            Line = $index + 1
            Index = $index
        })
    }
}

if ($sections.Count -eq 0) {
    throw "No canonical numbered sections found"
}

$duplicates = $sections | Group-Object Number | Where-Object Count -gt 1
if ($duplicates) {
    throw "Duplicate canonical section numbers: $($duplicates.Name -join ', ')"
}

$ordered = $sections | Sort-Object Number
for ($expected = 0; $expected -le $ordered[-1].Number; $expected++) {
    if ($ordered[$expected].Number -ne $expected) {
        throw "Canonical section sequence gap at $expected"
    }
}

$outputLines = [System.Collections.Generic.List[string]]::new()
$outputLines.Add("id`tname`tpurpose`tparent`tdependencies`tinterfaces`timplementation_files`ttests`tbenchmarks`tscore_0_100`tstatus`tversion`tevidence`tprovenance`tlast_verified`taliases")

for ($sectionIndex = 0; $sectionIndex -lt $ordered.Count; $sectionIndex++) {
    $section = $ordered[$sectionIndex]
    $nextIndex = if ($sectionIndex + 1 -lt $ordered.Count) { $ordered[$sectionIndex + 1].Index } else { $lines.Count }
    $purpose = ""
    for ($lineIndex = $section.Index + 1; $lineIndex -lt $nextIndex; $lineIndex++) {
        $candidate = $lines[$lineIndex].Trim()
        if ([string]::IsNullOrWhiteSpace($candidate) -or $candidate -eq '---' -or $candidate.StartsWith('```') -or $candidate.StartsWith('#')) {
            continue
        }
        $purpose = Normalize-Field $candidate
        break
    }
    $id = 'CANON-{0:D4}' -f $section.Number
    $parent = Get-Domain $section.Title
    $fields = @(
        $id,
        (Normalize-Field $section.Title),
        $purpose,
        $parent,
        '-', '-', '-', '-', '-',
        '0',
        'DISCOVERED',
        '1.0.0',
        '-',
        "docs/specifications/source/genesis.txt:$($section.Line)",
        '2026-09-02',
        "SECTION-$($section.Number)"
    )
    $outputLines.Add(($fields -join "`t"))
}

$outputParent = Split-Path -Parent $Output
if ($outputParent) {
    New-Item -ItemType Directory -Path $outputParent -Force | Out-Null
}
$outputLines | Set-Content -LiteralPath $Output -Encoding utf8NoBOM
Write-Output "Generated $($ordered.Count) canonical source-section rows in $Output"
