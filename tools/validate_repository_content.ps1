param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
function Fail([string]$Message) { throw "Repository content validation failed: $Message" }

$manifestPath = Join-Path $Root 'registry/repository_content.tsv'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { Fail 'repository_content.tsv is missing' }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ("genesis-repository-content-{0}.tsv" -f [guid]::NewGuid().ToString('N'))
try {
    & (Join-Path $Root 'tools/inventory_repository_content.ps1') -Root $Root -Output $temporary -AuditDate '2026-09-06' | Out-Null
    $expected = [IO.File]::ReadAllBytes($manifestPath)
    $actual = [IO.File]::ReadAllBytes($temporary)
    if ($expected.Length -ne $actual.Length -or -not [Linq.Enumerable]::SequenceEqual[byte]($expected, $actual)) {
        Fail 'manifest is stale; run tools/inventory_repository_content.ps1 after all source changes'
    }
    $rows = @(Import-Csv -Delimiter "`t" -LiteralPath $manifestPath)
    if ($rows.Count -eq 0) { Fail 'manifest is empty' }
    foreach ($column in @('path','bytes','lines','extension','media_class','category','role','hash_mode','sha256','audited_at')) {
        if ($rows[0].PSObject.Properties.Name -notcontains $column) { Fail "missing column $column" }
    }
    $duplicates = @($rows | Group-Object path | Where-Object Count -gt 1)
    if ($duplicates) { Fail "duplicate paths: $($duplicates.Name -join ', ')" }
    foreach ($row in $rows) {
        if ($row.path.Contains('\') -or $row.path.Contains('..') -or [IO.Path]::IsPathRooted($row.path)) { Fail "unsafe path $($row.path)" }
        if ($row.sha256 -notmatch '^[0-9A-F]{64}$') { Fail "$($row.path) has invalid SHA-256" }
        if ($row.hash_mode -notin @('CANONICAL_LF_SHA256','RAW_SHA256')) { Fail "$($row.path) has invalid hash mode" }
        if ($row.media_class -notin @('text','image','binary')) { Fail "$($row.path) has invalid media class" }
        if ($row.audited_at -ne '2026-09-06') { Fail "$($row.path) has an unexpected audit date" }
    }
    $bytes = ($rows | Measure-Object -Property bytes -Sum).Sum
    $text = @($rows | Where-Object media_class -eq 'text').Count
    $images = @($rows | Where-Object media_class -eq 'image').Count
    Write-Output "Repository content validation passed: $($rows.Count) canonical files, $bytes bytes, $text text files, $images images"
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
}
