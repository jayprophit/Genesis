param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
function Fail([string]$Message) { throw "ChatGPT project intake validation failed: $Message" }

$path = Join-Path $Root 'registry/chatgpt_genesis_conversations.tsv'
if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { Fail 'conversation registry is missing' }
$rows = @(Import-Csv -Delimiter "`t" -LiteralPath $path)
$columns = @('source_id','pages_read','turns_read','items_read','user_chars_observed','assistant_chars_observed','truncated_items','attachments_observed','links_observed','topic_signals','traversal_status','review_status','disposition','locator_status','reviewed_at')
if ($rows.Count -ne 47) { Fail "expected 47 conversations, found $($rows.Count)" }
foreach ($column in $columns) { if ($rows[0].PSObject.Properties.Name -notcontains $column) { Fail "missing column $column" } }
$unexpectedPrivateColumns = @('conversation_id','title','project_id','source_ref') | Where-Object { $rows[0].PSObject.Properties.Name -contains $_ }
if ($unexpectedPrivateColumns) { Fail "public registry exposes private locator metadata: $($unexpectedPrivateColumns -join ', ')" }
$duplicates = @($rows | Group-Object source_id | Where-Object Count -gt 1)
if ($duplicates) { Fail "duplicate source_id values: $($duplicates.Name -join ', ')" }

$allowedReviews = @('PRIORITY_ABSTRACTED','INDEXED_AND_CLASSIFIED')
$allowedDispositions = @('DESIGN_INPUT','RESEARCH_ONLY','APPLICATION_CANDIDATE','LEGACY_REFERENCE','META_INTAKE','GOVERNANCE_RESEARCH','AGENT_INPUT','ARCHITECTURE_INPUT')
$numericFields = @('pages_read','turns_read','items_read','user_chars_observed','assistant_chars_observed','truncated_items','attachments_observed','links_observed')
for ($index = 0; $index -lt $rows.Count; ++$index) {
    $row = $rows[$index]
    if ($row.source_id -ne ('CGP-{0:D3}' -f ($index + 1))) { Fail "source sequence changed at $($row.source_id)" }
    if ($row.locator_status -ne 'PRIVATE_LOCAL_ONLY') { Fail "$($row.source_id) does not protect its cloud locator" }
    if ($row.traversal_status -ne 'OLDEST_PAGE_REACHED') { Fail "$($row.source_id) is not fully paginated" }
    if ($allowedReviews -notcontains $row.review_status) { Fail "$($row.source_id) has invalid review status" }
    if ($allowedDispositions -notcontains $row.disposition) { Fail "$($row.source_id) has invalid disposition" }
    if ($row.topic_signals -notmatch '^[a-z_]+(,[a-z_]+){0,2}$') { Fail "$($row.source_id) has invalid topic signals" }
    if ($row.reviewed_at -ne '2026-09-06') { Fail "$($row.source_id) has an unexpected review date" }
    foreach ($field in $numericFields) {
        [int64]$value = 0
        if (-not [int64]::TryParse($row.$field, [ref]$value) -or $value -lt 0) { Fail "$($row.source_id) has invalid $field" }
        if ($field -in @('pages_read','turns_read','items_read') -and $value -eq 0) { Fail "$($row.source_id) has empty $field" }
    }
    if ([int64]$row.truncated_items -gt [int64]$row.items_read) { Fail "$($row.source_id) truncation count exceeds items" }
}

$totals = @{
    pages_read = 264; turns_read = 2400; items_read = 4522;
    user_chars_observed = 528102; assistant_chars_observed = 1395784;
    truncated_items = 994; attachments_observed = 7; links_observed = 206
}
foreach ($field in $totals.Keys) {
    $actual = ($rows | Measure-Object -Property $field -Sum).Sum
    if ([int64]$actual -ne [int64]$totals[$field]) { Fail "$field total changed: $actual" }
}

Write-Output 'ChatGPT project intake validation passed: 47 conversations, 264 pages, 2400 turns, 4522 items; 994 bounded-reader truncations remain explicit'
