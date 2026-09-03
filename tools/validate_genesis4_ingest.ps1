[CmdletBinding()]
param([string]$Root=(Split-Path -Parent $PSScriptRoot),[string]$ExternalSource='C:\Users\jpowe\Documents\genesis4.txt')
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'validate_continuation_ingest.ps1') -Root $Root -Label 'Genesis4' -RepositorySource 'docs/specifications/source/genesis4.txt' -Index 'registry/genesis4_sections.tsv' -ExternalSource $ExternalSource -ExpectedBytes 229615 -ExpectedHash 'D61A862275B620977E8E63BA4BEB39E776799C303D0E3533F8094635B8D7DC92' -ExpectedLines 9534 -ExpectedHeadings 581 -Prefix 'G4'
$lines=[IO.File]::ReadAllLines((Join-Path $Root 'docs/specifications/source/genesis4.txt'))
for($index=0;$index -lt 2901;++$index){if($lines[$index] -cne $lines[$index+2901]){throw "Genesis4 expected duplicate blocks differ at lines $($index+1) and $($index+2902)"}}
$block=($lines[0..2900] -join "`r`n")+"`r`n"
$blockHash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($block)))
if($blockHash -ne '35D9F891148A5B80FA35D7B429323437BBC66F24DB0047D9B88C45B158823519'){throw 'Genesis4 duplicate-block digest changed'}
Write-Output 'Genesis4 internal deduplication validation passed: lines 1-2901 exactly equal lines 2902-5802.'
