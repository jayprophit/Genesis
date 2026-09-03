[CmdletBinding()]
param([string]$Source='docs/specifications/source/genesis4.txt',[string]$Output='registry/genesis4_sections.tsv')
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'generate_continuation_registry.ps1') -Source $Source -Output $Output -Prefix 'G4'
