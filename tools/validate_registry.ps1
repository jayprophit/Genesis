param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"

function Read-Tsv([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing registry file: $Path"
    }
    return @(Import-Csv -Delimiter "`t" -LiteralPath $Path)
}

function Require-Columns($Rows, [string[]]$Columns, [string]$Name) {
    if ($Rows.Count -eq 0) { throw "$Name is empty" }
    foreach ($column in $Columns) {
        if (-not ($Rows[0].PSObject.Properties.Name -contains $column)) {
            throw "$Name is missing required column $column"
        }
    }
}

function Assert-Unique($Rows, [string]$Property, [string]$Name) {
    $duplicates = @($Rows | Group-Object -Property $Property | Where-Object Count -gt 1)
    if ($duplicates.Count -gt 0) {
        throw "$Name has duplicate $Property values: $($duplicates.Name -join ', ')"
    }
}

$requirements = Read-Tsv (Join-Path $Root "registry/requirements.tsv")
$domains = Read-Tsv (Join-Path $Root "registry/domains.tsv")
$canonical = Read-Tsv (Join-Path $Root "registry/canonical_sections.tsv")
Require-Columns $requirements @("id","name","purpose","parent","dependencies","interfaces","implementation_files","tests","benchmarks","score_0_100","status","version","evidence","provenance","last_verified","aliases") "requirements"
Require-Columns $canonical @("id","name","purpose","parent","dependencies","interfaces","implementation_files","tests","benchmarks","score_0_100","status","version","evidence","provenance","last_verified","aliases") "canonical sections"
Assert-Unique $requirements "id" "requirements"
Assert-Unique $canonical "id" "canonical sections"

$domainIds = @{}
foreach ($domain in $domains) { $domainIds[$domain.id] = $true }
$requirementIds = @{}
foreach ($requirement in $requirements) { $requirementIds[$requirement.id] = $true }

$allowedStatuses = @("DISCOVERED","SPECIFIED","SCAFFOLDED","IMPLEMENTED","COMPILED","UNIT_TESTED","INTEGRATION_TESTED","BENCHMARKED","PROVEN","OPTIMIZED","STABLE","SUPERSEDED")
foreach ($requirement in $requirements) {
    if (-not $domainIds.ContainsKey($requirement.parent)) { throw "$($requirement.id) has unknown parent $($requirement.parent)" }
    if ($allowedStatuses -notcontains $requirement.status) { throw "$($requirement.id) has unknown status $($requirement.status)" }
    $score = 0
    if (-not [int]::TryParse($requirement.score_0_100, [ref]$score) -or $score -lt 0 -or $score -gt 100) { throw "$($requirement.id) has invalid score" }
    if ($requirement.dependencies -and $requirement.dependencies -ne "-") {
        foreach ($dependency in $requirement.dependencies.Split(',')) {
            if (-not $requirementIds.ContainsKey($dependency)) { throw "$($requirement.id) has missing dependency $dependency" }
        }
    }
}

if ($canonical.Count -ne 1451) { throw "Canonical section count is $($canonical.Count), expected 1451" }
for ($index = 0; $index -lt $canonical.Count; $index++) {
    $expected = "CANON-{0:D4}" -f $index
    if ($canonical[$index].id -ne $expected) { throw "Canonical section sequence mismatch at $index" }
    if ($canonical[$index].status -ne "DISCOVERED") { throw "$expected must remain DISCOVERED until atomized and evidenced" }
    if (-not $domainIds.ContainsKey($canonical[$index].parent)) { throw "$expected has unknown domain" }
}

$sourcePath = Join-Path $Root "docs/specifications/source/genesis.txt"
$sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $sourcePath).Hash.ToUpperInvariant()
$sourceBytes = (Get-Item -LiteralPath $sourcePath).Length
if ($sourceHash -ne "6C9424AC87C3363CB194206C06B2C6BB093D8BBD57EDB8B8CF8466B0358C0BAA" -or $sourceBytes -ne 569220) {
    throw "Canonical source checksum or byte count changed unexpectedly"
}

$manifestRows = Read-Tsv (Join-Path $Root "provenance/SOURCE_MANIFEST.tsv")
$manifestSource = $manifestRows | Where-Object source_id -eq "SRC-GENESIS-SPEC"
if ($null -eq $manifestSource -or $manifestSource.sha256 -ne $sourceHash -or [int64]$manifestSource.bytes -ne $sourceBytes) {
    throw "Canonical source manifest entry does not match the preserved source"
}

Write-Output "Registry validation passed: $($requirements.Count) implementation requirements, $($canonical.Count) canonical source sections, $($domains.Count) domains"

