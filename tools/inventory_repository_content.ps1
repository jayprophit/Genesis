[CmdletBinding()]
param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot),
    [string]$Output = 'registry/repository_content.tsv',
    [string]$AuditDate = '2026-09-06'
)

$ErrorActionPreference = 'Stop'
$rootFull = [IO.Path]::GetFullPath($Root).TrimEnd([IO.Path]::DirectorySeparatorChar)
if (-not (Test-Path -LiteralPath $rootFull -PathType Container)) { throw "Repository root does not exist: $rootFull" }

function Is-CanonicalText([string]$RelativePath) {
    $extension = [IO.Path]::GetExtension($RelativePath).ToLowerInvariant()
    $name = [IO.Path]::GetFileName($RelativePath)
    return $extension -in @('.cpp','.hpp','.md','.ps1','.json','.tsv','.yml','.yaml','.cmake') -or
           $name -in @('CMakeLists.txt','CMakePresets.json','.gitignore','.gitattributes','LICENSE')
}
function Canonical-LfBytes([byte[]]$Bytes) {
    $output = [Collections.Generic.List[byte]]::new($Bytes.Length)
    for ($index = 0; $index -lt $Bytes.Length; ++$index) {
        if ($Bytes[$index] -eq 13 -and $index + 1 -lt $Bytes.Length -and $Bytes[$index + 1] -eq 10) { continue }
        $output.Add($Bytes[$index])
    }
    return $output.ToArray()
}
function Sha256([byte[]]$Bytes) {
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($Bytes))
}
function Line-Count([byte[]]$Bytes) {
    if ($Bytes.Length -eq 0) { return 0 }
    [int64]$lines = 0
    foreach ($value in $Bytes) { if ($value -eq 10) { ++$lines } }
    if ($Bytes[$Bytes.Length - 1] -ne 10) { ++$lines }
    return $lines
}
function Role([string]$Path) {
    switch -Regex ($Path) {
        '^include/' { return 'PUBLIC_INTERFACE' }
        '^src/' { return 'RUNTIME_IMPLEMENTATION' }
        '^tests/' { return 'TEST_EVIDENCE' }
        '^benchmarks/' { return 'BENCHMARK_EVIDENCE' }
        '^tools/' { return 'REPOSITORY_TOOLING' }
        '^registry/' { return 'MACHINE_REGISTRY' }
        '^provenance/' { return 'PROVENANCE_EVIDENCE' }
        '^assets/' { return 'GOVERNED_ASSET' }
        '^docs/specifications/source/' { return 'PRESERVED_SOURCE' }
        '^docs/' { return 'DOCUMENTATION' }
        '^research/' { return 'RESEARCH_EVIDENCE' }
        '^schemas/' { return 'DATA_SCHEMA' }
        '^templates/' { return 'PROJECT_TEMPLATE' }
        '^\.github/' { return 'CI_CONFIGURATION' }
        default { return 'PROJECT_CONFIGURATION' }
    }
}

$rows = [Collections.Generic.List[object]]::new()
foreach ($item in Get-ChildItem -LiteralPath $rootFull -File -Recurse -Force) {
    $relative = [IO.Path]::GetRelativePath($rootFull, $item.FullName).Replace('\','/')
    if ($relative -match '^(\.git|\.genesis-local|build(?:-[^/]+)?)(/|$)' -or
        $relative -eq 'registry/repository_content.tsv' -or
        $relative -like 'docs/audit/REPOSITORY_CONTENT_AUDIT_*.md') { continue }
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Repository source may not be a reparse point: $relative" }
    $raw = [IO.File]::ReadAllBytes($item.FullName)
    $canonicalText = Is-CanonicalText $relative
    $hashBytes = if ($canonicalText) { Canonical-LfBytes $raw } else { $raw }
    $extension = [IO.Path]::GetExtension($relative).ToLowerInvariant()
    if ([string]::IsNullOrEmpty($extension)) { $extension = '-' }
    $mediaClass = if ($extension -in @('.png','.svg')) { 'image' } elseif ($canonicalText -or $extension -eq '.txt') { 'text' } else { 'binary' }
    $top = if ($relative.Contains('/')) { $relative.Split('/')[0] } else { 'ROOT' }
    $rows.Add([pscustomobject]@{
        path = $relative
        bytes = $item.Length
        lines = if ($mediaClass -eq 'text') { Line-Count $raw } else { 0 }
        extension = $extension
        media_class = $mediaClass
        category = $top
        role = Role $relative
        hash_mode = if ($canonicalText) { 'CANONICAL_LF_SHA256' } else { 'RAW_SHA256' }
        sha256 = Sha256 $hashBytes
        audited_at = $AuditDate
    })
}

$rows = @($rows | Sort-Object path)
$lines = [Collections.Generic.List[string]]::new()
$lines.Add("path`tbytes`tlines`textension`tmedia_class`tcategory`trole`thash_mode`tsha256`taudited_at")
foreach ($row in $rows) {
    $lines.Add(($row.path,$row.bytes,$row.lines,$row.extension,$row.media_class,$row.category,$row.role,$row.hash_mode,$row.sha256,$row.audited_at -join "`t"))
}
$outputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path $rootFull $Output }
$outputDirectory = Split-Path -Parent $outputPath
if (-not (Test-Path -LiteralPath $outputDirectory)) { New-Item -ItemType Directory -Path $outputDirectory | Out-Null }
[IO.File]::WriteAllLines($outputPath, $lines, [Text.UTF8Encoding]::new($false))
Write-Output "Repository content inventory wrote $($rows.Count) canonical source files to $outputPath"
