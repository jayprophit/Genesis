<#
.SYNOPSIS
  Proves the repository-content inventory is a fact about committed content.

.DESCRIPTION
  Two properties, each of which was a real defect:

  1. Untracked and generated files must not affect the inventory. The previous
     inventory walked the filesystem, so it recorded 20 untracked __pycache__ .pyc
     files and any untracked third-party reference tree, which made the recorded
     audit depend on the machine that produced it and impossible to reproduce
     from a clean clone.
  2. Repeated runs must be byte-identical. The previous culture-sensitive sort
     tied 'affect.hpp' and 'affect_persistence.hpp' at the primary weight, so
     their order depended on enumeration order and the same content serialized
     two different ways.

  The test creates untracked noise, regenerates the inventory, and requires it to
  be byte-identical to the committed manifest. If the inventory ever drifts back
  to describing the working tree, this fails.
#>
[CmdletBinding()]
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$manifestPath = Join-Path $Root 'registry/repository_content.tsv'
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw 'repository_content.tsv is missing'
}
$inventory = Join-Path $Root 'tools/inventory_repository_content.ps1'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('genesis-inventory-test-{0}' -f [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $scratch | Out-Null
$noise = @(
    (Join-Path $Root ('integration-archive/genesis_runtime/__pycache__/inventory_probe.cpython-313.pyc')),
    (Join-Path $Root 'references/inventory_probe_untracked.md')
)
$created = [Collections.Generic.List[string]]::new()

# NOTE: this deliberately does not use a helper function. PowerShell unrolls an
# array returned from a function, so a `return [byte[]]` helper hands back an
# Object[] and the byte comparison silently stops type-matching. The reads stay
# inline and explicit instead.
& $inventory -Root $Root -Output (Join-Path $scratch 'first.tsv') -AuditDate '2026-09-06' | Out-Null
$committed = [IO.File]::ReadAllBytes($manifestPath)
$first = [IO.File]::ReadAllBytes((Join-Path $scratch 'first.tsv'))
if (-not [Linq.Enumerable]::SequenceEqual[byte]($first, $committed)) {
    throw 'inventory does not reproduce the committed manifest (run tools/inventory_repository_content.ps1)'
}

try {
    # Property 1: untracked, generated and third-party-looking files must not
    # change the recorded audit.
    foreach ($path in $noise) {
        $directory = Split-Path -Parent $path
        if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
            New-Item -ItemType Directory -Path $directory -Force | Out-Null
        }
        [IO.File]::WriteAllBytes($path, [Text.Encoding]::UTF8.GetBytes("probe $path"))
        $created.Add($path)
    }
    & $inventory -Root $Root -Output (Join-Path $scratch 'second.tsv') -AuditDate '2026-09-06' | Out-Null
    $withNoise = [IO.File]::ReadAllBytes((Join-Path $scratch 'second.tsv'))
    if (-not [Linq.Enumerable]::SequenceEqual[byte]($withNoise, $committed)) {
        throw 'untracked files changed the inventory; it is describing the working tree, not the repository'
    }
    foreach ($path in $created) { Remove-Item -LiteralPath $path -Force }
    $created.Clear()

    # Property 2: determinism, twice in a row with no input change at all.
    & $inventory -Root $Root -Output (Join-Path $scratch 'third.tsv') -AuditDate '2026-09-06' | Out-Null
    $again = [IO.File]::ReadAllBytes((Join-Path $scratch 'third.tsv'))
    if (-not [Linq.Enumerable]::SequenceEqual[byte]($again, $committed)) {
        throw 'repeated inventory runs disagree; the ordering is not deterministic'
    }

    # Property 3: the emitted row order is canonical. The ordering defect was
    # invisible while one enumeration order was used everywhere: git returns
    # paths in byte order and a filesystem walk does not, and a culture-sensitive
    # sort ties 'affect.hpp' against 'affect_persistence.hpp', so their relative
    # order silently depended on which order arrived first. The manifest must
    # therefore be in strict ordinal order - which is what the generator's
    # ordinal sort guarantees and a culture sort does not.
    $rows = @(Import-Csv -Delimiter "`t" -LiteralPath $manifestPath)
    $manifestOrder = [string[]]@($rows | ForEach-Object { $_.path })
    $ordinalOrder = [string[]]@($manifestOrder.Clone())
    [Array]::Sort($ordinalOrder, [StringComparer]::Ordinal)
    for ($i = 0; $i -lt $manifestOrder.Length; $i++) {
        if ($manifestOrder[$i] -cne $ordinalOrder[$i]) {
            throw ("the manifest is not in canonical ordinal order: position {0} is '{1}' but ordinal order requires '{2}'" -f `
                $i, $manifestOrder[$i], $ordinalOrder[$i])
        }
    }
    # A culture sort is a different order; if the manifest ever matched it
    # instead, the row set would differ at the first ambiguous pair.
    $cultureOrder = [string[]]@($manifestOrder | Sort-Object)
    $differsFromCulture = $false
    for ($i = 0; $i -lt $manifestOrder.Length; $i++) {
        if ($manifestOrder[$i] -cne $cultureOrder[$i]) { $differsFromCulture = $true; break }
    }
    if (-not $differsFromCulture) {
        throw 'ordinal and culture ordering agree here, so this manifest cannot distinguish the sorting defect; the corpus needs a case like affect.hpp vs affect_persistence.hpp'
    }

    # The manifest must not describe generated bytecode, whatever happens.
    $rows = @(Import-Csv -Delimiter "`t" -LiteralPath $manifestPath)
    $bytecode = @($rows | Where-Object { $_.extension -eq '.pyc' })
    if ($bytecode.Count -gt 0) {
        throw "manifest records compiled bytecode: $($bytecode.path -join ', ')"
    }
    Write-Output ("Repository content inventory test passed: {0} tracked files, untracked files ignored, deterministic ordering" -f $rows.Count)
} finally {
    foreach ($path in $created) {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
    }
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
}
