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
$research = Read-Tsv (Join-Path $Root "registry/research_items.tsv")
$researchClasses = Read-Tsv (Join-Path $Root "registry/research_classes.tsv")
$deadEnds = Read-Tsv (Join-Path $Root "registry/research_dead_ends.tsv")
Require-Columns $requirements @("id","name","purpose","parent","dependencies","interfaces","implementation_files","tests","benchmarks","score_0_100","status","version","evidence","provenance","last_verified","aliases") "requirements"
Require-Columns $canonical @("id","name","purpose","parent","dependencies","interfaces","implementation_files","tests","benchmarks","score_0_100","status","version","evidence","provenance","last_verified","aliases") "canonical sections"
Require-Columns $research @("research_id","title","source_type","source_name","creator","year","domain","technology_category","description","claimed_function","underlying_mechanism","evidence_class","utility_class","known_physics_status","real_world_analogues","related_patents","related_papers","related_fiction","modules_affected","useful_principles","failure_lessons","prototype_possible","simulation_possible","tests","results","status","reason_accepted","reason_rejected","reopen_conditions","provenance","last_reviewed") "research items"
Require-Columns $researchClasses @("class_type","code","name","meaning","provenance","last_reviewed") "research classes"
Require-Columns $deadEnds @("dead_end_id","research_id","idea","source","date_investigated","reason_investigated","evidence_found","tests_performed","reason_rejected","useful_fragments","reopen_condition","related_ideas","status","provenance","last_reviewed") "research dead ends"
Assert-Unique $requirements "id" "requirements"
Assert-Unique $canonical "id" "canonical sections"
Assert-Unique $research "research_id" "research items"
Assert-Unique $researchClasses "code" "research classes"
Assert-Unique $deadEnds "dead_end_id" "research dead ends"

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

$allowedResearchSourceTypes = @("FICTIONAL","SCIENTIFIC","PATENT","UAP","SPECULATIVE","PRINCIPLE","STANDARD","HISTORICAL")
$allowedEvidenceClasses = 0..7 | ForEach-Object { "E$_" }
$allowedUtilityClasses = 0..6 | ForEach-Object { "U$_" }
$allowedResearchStatuses = @("REGISTERED","CANDIDATE","IN_PROGRESS","ACCEPTED","DEFERRED","REJECTED","SUPERSEDED","OPEN")
$researchIds = @{}
foreach ($item in $research) {
    if ($item.research_id -match "\s" -or [string]::IsNullOrWhiteSpace($item.research_id)) { throw "research item has an invalid ID" }
    if ($allowedResearchSourceTypes -notcontains $item.source_type) { throw "$($item.research_id) has unknown source type $($item.source_type)" }
    if ($allowedEvidenceClasses -notcontains $item.evidence_class) { throw "$($item.research_id) has unknown evidence class $($item.evidence_class)" }
    if ($allowedUtilityClasses -notcontains $item.utility_class) { throw "$($item.research_id) has unknown utility class $($item.utility_class)" }
    if ($allowedResearchStatuses -notcontains $item.status) { throw "$($item.research_id) has unknown research status $($item.status)" }
    foreach ($field in @("title","source_name","creator","domain","technology_category","description","claimed_function","underlying_mechanism","modules_affected","useful_principles","failure_lessons","prototype_possible","simulation_possible","status","reason_accepted","reopen_conditions","provenance","last_reviewed")) {
        if ([string]::IsNullOrWhiteSpace($item.$field)) { throw "$($item.research_id) is missing $field" }
    }
    $researchIds[$item.research_id] = $true
}
$classCodes = @{}
foreach ($class in $researchClasses) {
    if ($class.class_type -notin @("EVIDENCE","UTILITY")) { throw "research class $($class.code) has unknown type $($class.class_type)" }
    if ([string]::IsNullOrWhiteSpace($class.name) -or [string]::IsNullOrWhiteSpace($class.meaning)) { throw "research class $($class.code) is incomplete" }
    $classCodes[$class.code] = $class.class_type
}
foreach ($evidence in $allowedEvidenceClasses) {
    if ($classCodes[$evidence] -ne "EVIDENCE") { throw "missing evidence class definition $evidence" }
}
foreach ($utility in $allowedUtilityClasses) {
    if ($classCodes[$utility] -ne "UTILITY") { throw "missing utility class definition $utility" }
}
$requiredResearchIds = @("RES-UNIVERSAL","RES-FICTION","RES-SCIFI","RES-PATENT","RES-PATENT-INTELLIGENCE","RES-UAP","RES-SPECULATION","RES-TECH-RADAR","RES-DEADEND","SCI-ANOMALY","COG-PERSPECTIVE-COUNCIL","DAT-SHARED-DOMAIN","SIM-DIVERGENCE","SLF-NET-BOUNDARY")
foreach ($researchId in $requiredResearchIds) {
    if (-not $researchIds.ContainsKey($researchId)) { throw "required research item $researchId is missing" }
}
$allowedDeadEndStatuses = @("OPEN","DEFERRED","REOPENED","REJECTED_CURRENTLY","INSUFFICIENT_EVIDENCE","FAILED_PROTOTYPE","PHYSICALLY_UNSUPPORTED","COMPUTATIONALLY_INEFFICIENT","DUPLICATES_EXISTING_SYSTEM","USELESS_FOR_GENESIS","RESEARCH_AGAIN_IF_NEW_EVIDENCE")
foreach ($deadEnd in $deadEnds) {
    if (-not $researchIds.ContainsKey($deadEnd.research_id)) { throw "$($deadEnd.dead_end_id) references missing research item $($deadEnd.research_id)" }
    if ($allowedDeadEndStatuses -notcontains $deadEnd.status) { throw "$($deadEnd.dead_end_id) has unknown dead-end status $($deadEnd.status)" }
    foreach ($field in @("idea","source","date_investigated","reason_investigated","evidence_found","tests_performed","reason_rejected","useful_fragments","reopen_condition","related_ideas","status","provenance","last_reviewed")) {
        if ([string]::IsNullOrWhiteSpace($deadEnd.$field)) { throw "$($deadEnd.dead_end_id) is missing $field" }
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

$technologyPath = Join-Path $Root "docs/specifications/source/technology_mining_addendum.txt"
$technologyHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $technologyPath).Hash.ToUpperInvariant()
$technologyBytes = (Get-Item -LiteralPath $technologyPath).Length
if ($technologyHash -ne "799564F127098B759F0EF3D6BAD76870E7027A00466591910B20AA6663B35803" -or $technologyBytes -ne 58224) {
    throw "Technology-mining addendum checksum or byte count changed unexpectedly"
}
$manifestTechnology = $manifestRows | Where-Object source_id -eq "SRC-GENESIS-TECH-MINING-COPY"
if ($null -eq $manifestTechnology -or $manifestTechnology.sha256 -ne $technologyHash -or [int64]$manifestTechnology.bytes -ne $technologyBytes) {
    throw "Technology-mining source manifest entry does not match the preserved addendum"
}

Write-Output "Registry validation passed: $($requirements.Count) implementation requirements, $($canonical.Count) canonical source sections, $($research.Count) research items, $($researchClasses.Count) research classes, $($deadEnds.Count) dead ends, $($domains.Count) domains"
