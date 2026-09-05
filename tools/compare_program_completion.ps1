[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BaseRef,
    [string]$TargetRef = 'WORKTREE'
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Read-TsvAtRef {
    param(
        [string]$Ref,
        [string]$Path
    )

    if ($Ref -eq 'WORKTREE') {
        return @(Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo $Path))
    }

    $resolved = (& git -C $repo rev-parse --verify "$Ref^{commit}" 2>$null)
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($resolved)) {
        throw "Git reference '$Ref' does not resolve to a commit"
    }
    $text = (& git -C $repo show "${resolved}:$Path" 2>$null) -join "`n"
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($text)) {
        throw "Cannot read '$Path' at '$Ref'"
    }
    return @($text | ConvertFrom-Csv -Delimiter "`t")
}

function Index-ById {
    param([object[]]$Rows)
    $index = @{}
    foreach ($row in $Rows) {
        if ($index.ContainsKey($row.id)) {
            throw "Duplicate id '$($row.id)'"
        }
        $index[$row.id] = $row
    }
    return $index
}

function Join-OrNone {
    param([object[]]$Values)
    if ($Values.Count -eq 0) { return '(none)' }
    return ($Values | Sort-Object) -join ', '
}

$baseRequirements = Read-TsvAtRef $BaseRef 'registry/requirements.tsv'
$targetRequirements = Read-TsvAtRef $TargetRef 'registry/requirements.tsv'
$baseComponents = Read-TsvAtRef $BaseRef 'registry/completion_components.tsv'
$targetComponents = Read-TsvAtRef $TargetRef 'registry/completion_components.tsv'
$baseReport = Read-TsvAtRef $BaseRef 'registry/completion_report.tsv'
$targetReport = Read-TsvAtRef $TargetRef 'registry/completion_report.tsv'

$baseRequirementIndex = Index-ById $baseRequirements
$targetRequirementIndex = Index-ById $targetRequirements
$baseComponentIndex = Index-ById $baseComponents
$targetComponentIndex = Index-ById $targetComponents
$baseReportIndex = Index-ById $baseReport
$targetReportIndex = Index-ById $targetReport

$addedRequirements = @($targetRequirementIndex.Keys | Where-Object {
        -not $baseRequirementIndex.ContainsKey($_)
    })
$removedRequirements = @($baseRequirementIndex.Keys | Where-Object {
        -not $targetRequirementIndex.ContainsKey($_)
    })
$addedComponents = @($targetComponentIndex.Keys | Where-Object {
        -not $baseComponentIndex.ContainsKey($_)
    })
$removedComponents = @($baseComponentIndex.Keys | Where-Object {
        -not $targetComponentIndex.ContainsKey($_)
    })

"Completion comparison: $BaseRef -> $TargetRef"
"Requirements: $($baseRequirements.Count) -> $($targetRequirements.Count)"
"  Added: $(Join-OrNone $addedRequirements)"
"  Removed: $(Join-OrNone $removedRequirements)"
"Components: $($baseComponents.Count) -> $($targetComponents.Count)"
"  Added: $(Join-OrNone $addedComponents)"
"  Removed: $(Join-OrNone $removedComponents)"

''
'Top-level percentage changes:'
$topLevelIds = @($baseReport | Where-Object parent -eq 'GENESIS' | ForEach-Object id)
$topLevelIds += @($targetReport | Where-Object parent -eq 'GENESIS' | ForEach-Object id)
foreach ($id in $topLevelIds | Sort-Object -Unique) {
    $before = if ($baseReportIndex.ContainsKey($id)) {
        $baseReportIndex[$id].percentage
    } else { '(absent)' }
    $after = if ($targetReportIndex.ContainsKey($id)) {
        $targetReportIndex[$id].percentage
    } else { '(absent)' }
    if ($before -ne $after) {
        "  $id`: $before -> $after"
    }
}
$baseTotal = if ($baseReportIndex.ContainsKey('GENESIS')) {
    $baseReportIndex['GENESIS'].percentage
} else { '(absent)' }
$targetTotal = if ($targetReportIndex.ContainsKey('GENESIS')) {
    $targetReportIndex['GENESIS'].percentage
} else { '(absent)' }
"Genesis total: $baseTotal -> $targetTotal"

''
'Existing requirement score/status changes:'
$requirementChanges = 0
foreach ($id in $baseRequirementIndex.Keys | Sort-Object) {
    if (-not $targetRequirementIndex.ContainsKey($id)) { continue }
    $before = $baseRequirementIndex[$id]
    $after = $targetRequirementIndex[$id]
    if ($before.score_0_100 -ne $after.score_0_100 -or
        $before.status -ne $after.status) {
        "  $id`: $($before.score_0_100)/$($before.status) -> $($after.score_0_100)/$($after.status)"
        $requirementChanges++
    }
}
if ($requirementChanges -eq 0) { '  (none)' }
