[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Root,
    [Parameter(Mandatory)][string]$Label,
    [Parameter(Mandatory)][string]$RepositorySource,
    [Parameter(Mandatory)][string]$Index,
    [Parameter(Mandatory)][string]$ExternalSource,
    [Parameter(Mandatory)][long]$ExpectedBytes,
    [Parameter(Mandatory)][string]$ExpectedHash,
    [Parameter(Mandatory)][int]$ExpectedLines,
    [Parameter(Mandatory)][int]$ExpectedHeadings,
    [Parameter(Mandatory)][ValidatePattern('^[A-Z][A-Z0-9]*$')][string]$Prefix
)

$ErrorActionPreference = 'Stop'
$copy = Join-Path $Root $RepositorySource
$indexPath = Join-Path $Root $Index
$ExpectedHash = $ExpectedHash.ToUpperInvariant()
if (-not (Test-Path -LiteralPath $copy)) { throw "$Label repository copy is missing" }
if ((Get-Item -LiteralPath $copy).Length -ne $ExpectedBytes) { throw "$Label repository copy byte count changed" }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $copy).Hash -ne $ExpectedHash) { throw "$Label repository copy hash changed" }
if (Test-Path -LiteralPath $ExternalSource) {
    if ((Get-Item -LiteralPath $ExternalSource).Length -ne $ExpectedBytes -or
        (Get-FileHash -Algorithm SHA256 -LiteralPath $ExternalSource).Hash -ne $ExpectedHash) {
        throw "External $Label source no longer matches the repository copy"
    }
}

$lines = [IO.File]::ReadAllLines($copy)
if ($lines.Count -ne $ExpectedLines) { throw "$Label physical line count changed" }
$headings = for ($i = 0; $i -lt $lines.Count; ++$i) {
    if ($lines[$i] -match '^#{1,6}\s+(.+?)\s*$') {
        [pscustomobject]@{ line = $i + 1; heading = ($Matches[1] -replace "`t", ' ') }
    }
}
$rows = @(Import-Csv -Delimiter "`t" -LiteralPath $indexPath)
if ($headings.Count -ne $ExpectedHeadings -or $rows.Count -ne $headings.Count) { throw "$Label heading index is incomplete" }
$ids = [Collections.Generic.HashSet[string]]::new()
for ($i = 0; $i -lt $rows.Count; ++$i) {
    $row = $rows[$i]
    $heading = $headings[$i]
    if (-not $ids.Add($row.id) -or $row.id -ne ('{0}-{1:D4}' -f $Prefix, ($i + 1))) { throw "$Label index ID sequence is invalid" }
    if ([int]$row.line -ne $heading.line -or $row.heading -ne $heading.heading) { throw "$Label index mismatch at $($row.id)" }
    $material = "$($heading.line):$($heading.heading)"
    $digest = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($material))).ToLowerInvariant()
    if ($row.sha256 -ne $digest -or $row.status -ne 'DISCOVERED') { throw "$Label digest or status mismatch at $($row.id)" }
}

$regenerated = [IO.Path]::GetTempFileName()
try {
    & (Join-Path $Root 'tools/generate_continuation_registry.ps1') -Source $copy -Output $regenerated -Prefix $Prefix | Out-Null
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $regenerated).Hash -ne
        (Get-FileHash -Algorithm SHA256 -LiteralPath $indexPath).Hash) {
        throw "$Label index is not reproducible from the preserved source"
    }
} finally {
    Remove-Item -LiteralPath $regenerated -Force -ErrorAction SilentlyContinue
}

Write-Output "$Label ingest validation passed: $ExpectedBytes bytes, SHA-256 $ExpectedHash, $ExpectedHeadings heading occurrences."
