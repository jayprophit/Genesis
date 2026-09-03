[CmdletBinding()]
param([string]$Root=(Split-Path -Parent $PSScriptRoot),[string]$ExternalSource='C:\Users\jpowe\Documents\Genesis3.txt')
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'validate_continuation_ingest.ps1') -Root $Root -Label 'Genesis3' -RepositorySource 'docs/specifications/source/genesis3.txt' -Index 'registry/genesis3_sections.tsv' -ExternalSource $ExternalSource -ExpectedBytes 392645 -ExpectedHash '00F9B2C6090DBC733A302AFDFEB41E929FF9A07F91942F73E9814A3FB3CE4319' -ExpectedLines 20564 -ExpectedHeadings 537 -Prefix 'G3'
