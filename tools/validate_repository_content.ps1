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
        # "The manifest is stale" is not a diagnosis. It says that
        # something differs and leaves the reader to find out what,
        # which on a second operating system means a bisect through a
        # six hundred row file. So the difference is named: the paths
        # whose rows differ, and how many.
        $expectedRows = @{}
        foreach ($row in @([IO.File]::ReadAllLines($manifestPath))) {
            $parts = $row -split "`t"
            if ($parts.Count -ge 2 -and $parts[0]) { $expectedRows[$parts[0]] = $row }
        }
        $differing = [Collections.Generic.List[string]]::new()
        $onlyInRegenerated = [Collections.Generic.List[string]]::new()
        foreach ($row in @([IO.File]::ReadAllLines($temporary))) {
            $parts = $row -split "`t"
            if ($parts.Count -lt 2 -or -not $parts[0]) { continue }
            if (-not $expectedRows.ContainsKey($parts[0])) {
                $onlyInRegenerated.Add($parts[0])
            } elseif ($expectedRows[$parts[0]] -ne $row) {
                $differing.Add($parts[0])
            }
        }
        $detail = "manifest is stale; $($differing.Count) row(s) differ and $($onlyInRegenerated.Count) path(s) are only in the regenerated manifest."
        if ($differing.Count -gt 0) {
            $detail += " The differing paths are: " + (($differing | Select-Object -First 12) -join ', ') + '.'
        }
        if ($onlyInRegenerated.Count -gt 0) {
            $detail += " The paths only in the regenerated manifest are: " + (($onlyInRegenerated | Select-Object -First 12) -join ', ') + '.'
        }
        $detail += " Run tools/inventory_repository_content.ps1 after all source changes."
        Fail $detail
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
