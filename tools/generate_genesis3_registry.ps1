[CmdletBinding()]
param([string]$Source='docs/specifications/source/genesis3.txt',[string]$Output='registry/genesis3_sections.tsv')
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'generate_continuation_registry.ps1') -Source $Source -Output $Output -Prefix 'G3'
