[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$requirements = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/requirements.tsv')
$sections = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/canonical_sections.tsv')
$implemented = @('UNIT_TESTED', 'INTEGRATION_TESTED', 'BENCHMARKED', 'QUALIFIED')
$counts = $requirements | Group-Object status | Sort-Object Name
Write-Output "Canonical sections indexed: $($sections.Count)"
Write-Output "Requirements registered: $($requirements.Count)"
foreach ($count in $counts) { Write-Output ("{0}: {1}" -f $count.Name, $count.Count) }
$verified = @($requirements | Where-Object { $_.status -in $implemented }).Count
Write-Output "Evidence-backed implemented requirements: $verified/$($requirements.Count)"
if ($verified -eq $requirements.Count -and $sections.Count -gt 0) {
    Write-Output 'Registry requirements are implemented; canonical section-by-section closure still requires an explicit coverage audit.'
} else {
    Write-Output 'Genesis specification is not yet fully implemented. Continue by dependency-ordered stage.'
}
