[CmdletBinding()]
param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot),
    [string]$ExternalSource = 'C:\Users\jpowe\Documents\Genesis3.txt'
)

$ErrorActionPreference = 'Stop'
$expectedBytes = 392645
$expectedHash = '00F9B2C6090DBC733A302AFDFEB41E929FF9A07F91942F73E9814A3FB3CE4319'
$copy = Join-Path $Root 'docs/specifications/source/genesis3.txt'
$index = Join-Path $Root 'registry/genesis3_sections.tsv'

if (-not (Test-Path -LiteralPath $copy)) { throw 'Genesis3 repository copy is missing' }
if ((Get-Item -LiteralPath $copy).Length -ne $expectedBytes) { throw 'Genesis3 repository copy byte count changed' }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $copy).Hash -ne $expectedHash) { throw 'Genesis3 repository copy hash changed' }
if (Test-Path -LiteralPath $ExternalSource) {
    if ((Get-Item -LiteralPath $ExternalSource).Length -ne $expectedBytes -or
        (Get-FileHash -Algorithm SHA256 -LiteralPath $ExternalSource).Hash -ne $expectedHash) {
        throw 'External Genesis3 source no longer matches the repository copy'
    }
}

$lines = [IO.File]::ReadAllLines($copy)
$headings = for ($i = 0; $i -lt $lines.Count; ++$i) {
    if ($lines[$i] -match '^#{1,6}\s+(.+?)\s*$') {
        [pscustomobject]@{ line = $i + 1; heading = ($Matches[1] -replace "`t", ' ') }
    }
}
$rows = @(Import-Csv -Delimiter "`t" -LiteralPath $index)
if ($headings.Count -ne 537 -or $rows.Count -ne $headings.Count) { throw 'Genesis3 heading index is incomplete' }
$ids = [Collections.Generic.HashSet[string]]::new()
for ($i = 0; $i -lt $rows.Count; ++$i) {
    $row = $rows[$i]
    $heading = $headings[$i]
    if (-not $ids.Add($row.id) -or $row.id -ne ('G3-{0:D4}' -f ($i + 1))) { throw 'Genesis3 index ID sequence is invalid' }
    if ([int]$row.line -ne $heading.line -or $row.heading -ne $heading.heading) { throw "Genesis3 index mismatch at $($row.id)" }
    $material = "$($heading.line):$($heading.heading)"
    $digest = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($material))).ToLowerInvariant()
    if ($row.sha256 -ne $digest -or $row.status -ne 'DISCOVERED') { throw "Genesis3 digest or status mismatch at $($row.id)" }
}

$regenerated = [IO.Path]::GetTempFileName()
try {
    & (Join-Path $Root 'tools/generate_genesis3_registry.ps1') -Source $copy -Output $regenerated | Out-Null
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $regenerated).Hash -ne
        (Get-FileHash -Algorithm SHA256 -LiteralPath $index).Hash) {
        throw 'Genesis3 index is not reproducible from the preserved source'
    }
} finally {
    Remove-Item -LiteralPath $regenerated -Force -ErrorAction SilentlyContinue
}

Write-Output "Genesis3 ingest validation passed: $expectedBytes bytes, SHA-256 $expectedHash, $($rows.Count) heading occurrences."
