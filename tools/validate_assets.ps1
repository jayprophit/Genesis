param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'

function Fail([string]$Message) { throw "Asset registry validation failed: $Message" }
function Required-Columns($Rows, [string[]]$Columns) {
    if ($Rows.Count -eq 0) { Fail 'registry is empty' }
    foreach ($column in $Columns) {
        if ($Rows[0].PSObject.Properties.Name -notcontains $column) {
            Fail "missing column $column"
        }
    }
}
function Read-BigEndianUInt32([byte[]]$Bytes, [int]$Offset) {
    return ([uint32]$Bytes[$Offset] -shl 24) -bor
           ([uint32]$Bytes[$Offset + 1] -shl 16) -bor
           ([uint32]$Bytes[$Offset + 2] -shl 8) -bor
           [uint32]$Bytes[$Offset + 3]
}

$registryPath = Join-Path $Root 'registry/assets.tsv'
if (-not (Test-Path -LiteralPath $registryPath -PathType Leaf)) { Fail 'registry/assets.tsv is missing' }
$rows = @(Import-Csv -Delimiter "`t" -LiteralPath $registryPath)
Required-Columns $rows @('asset_id','path','media_type','purpose','origin','creator','rights_status','sha256','bytes','width','height','status','claim_boundary','reviewed_at')

$duplicateIds = @($rows | Group-Object asset_id | Where-Object Count -gt 1)
$duplicatePaths = @($rows | Group-Object path | Where-Object Count -gt 1)
if ($duplicateIds) { Fail "duplicate asset IDs: $($duplicateIds.Name -join ', ')" }
if ($duplicatePaths) { Fail "duplicate asset paths: $($duplicatePaths.Name -join ', ')" }

$allowedMedia = @{
    '.svg' = 'image/svg+xml'; '.png' = 'image/png'; '.json' = 'application/json'; '.md' = 'text/markdown'
}
$allowedStatuses = @('INTERNAL_PROVISIONAL','INTERNAL_GUIDE','INTERNAL_PROVENANCE','INTERNAL_CONCEPT')
$assetRoot = [IO.Path]::GetFullPath((Join-Path $Root 'assets'))
$assetPrefix = $assetRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
$registeredPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)

foreach ($row in $rows) {
    if ($row.asset_id -notmatch '^ASSET-[A-Z0-9-]+$') { Fail "invalid asset ID $($row.asset_id)" }
    if ($row.path -notmatch '^assets/[A-Za-z0-9._/-]+$' -or $row.path.Contains('..')) { Fail "unsafe path $($row.path)" }
    if (-not $registeredPaths.Add($row.path)) { Fail "duplicate normalized path $($row.path)" }
    foreach ($field in @('purpose','origin','creator','rights_status','claim_boundary')) {
        if ([string]::IsNullOrWhiteSpace($row.$field)) { Fail "$($row.asset_id) is missing $field" }
    }
    if ($allowedStatuses -notcontains $row.status) { Fail "$($row.asset_id) has invalid status $($row.status)" }
    if ($row.reviewed_at -notmatch '^\d{4}-\d{2}-\d{2}$') { Fail "$($row.asset_id) has invalid review date" }
    if ($row.sha256 -notmatch '^[0-9A-F]{64}$') { Fail "$($row.asset_id) has invalid SHA-256" }
    [int64]$expectedBytes = 0
    if (-not [int64]::TryParse($row.bytes, [ref]$expectedBytes) -or $expectedBytes -le 0 -or $expectedBytes -gt 10MB) {
        Fail "$($row.asset_id) has invalid byte count"
    }
    $relativeNative = $row.path.Replace('/', [IO.Path]::DirectorySeparatorChar)
    $fullPath = [IO.Path]::GetFullPath((Join-Path $Root $relativeNative))
    if (-not $fullPath.StartsWith($assetPrefix, [StringComparison]::OrdinalIgnoreCase)) { Fail "$($row.asset_id) escapes assets/" }
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) { Fail "$($row.asset_id) file is missing" }
    $item = Get-Item -LiteralPath $fullPath
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Fail "$($row.asset_id) may not be a reparse point" }
    if ($item.Length -ne $expectedBytes) { Fail "$($row.asset_id) byte count changed" }
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fullPath).Hash
    if ($actualHash -ne $row.sha256) { Fail "$($row.asset_id) SHA-256 changed" }
    $extension = [IO.Path]::GetExtension($fullPath).ToLowerInvariant()
    if (-not $allowedMedia.ContainsKey($extension) -or $allowedMedia[$extension] -ne $row.media_type) {
        Fail "$($row.asset_id) media type does not match $extension"
    }

    if ($extension -eq '.png') {
        [int]$width = 0; [int]$height = 0
        if (-not [int]::TryParse($row.width, [ref]$width) -or -not [int]::TryParse($row.height, [ref]$height) -or $width -le 0 -or $height -le 0) {
            Fail "$($row.asset_id) has invalid PNG dimensions"
        }
        $bytes = [IO.File]::ReadAllBytes($fullPath)
        if ($bytes.Length -lt 24) { Fail "$($row.asset_id) is a truncated PNG" }
        $signature = ($bytes[0..7] | ForEach-Object { $_.ToString('X2') }) -join ''
        if ($signature -ne '89504E470D0A1A0A') { Fail "$($row.asset_id) has an invalid PNG signature" }
        if ((Read-BigEndianUInt32 $bytes 16) -ne $width -or (Read-BigEndianUInt32 $bytes 20) -ne $height) {
            Fail "$($row.asset_id) PNG dimensions changed"
        }
    } elseif ($extension -eq '.svg') {
        [int]$width = 0; [int]$height = 0
        if (-not [int]::TryParse($row.width, [ref]$width) -or -not [int]::TryParse($row.height, [ref]$height)) {
            Fail "$($row.asset_id) has invalid SVG dimensions"
        }
        $svg = Get-Content -Raw -LiteralPath $fullPath
        if ($svg -notmatch '<svg\b' -or $svg -notmatch '<title\b' -or $svg -notmatch '<desc\b') {
            Fail "$($row.asset_id) SVG lacks an accessible title or description"
        }
        if ($svg -notmatch 'viewBox\s*=\s*"[0-9.-]+\s+[0-9.-]+\s+([0-9.]+)\s+([0-9.]+)"') {
            Fail "$($row.asset_id) SVG lacks a canonical viewBox"
        }
        if ([int][double]$Matches[1] -ne $width -or [int][double]$Matches[2] -ne $height) {
            Fail "$($row.asset_id) SVG dimensions changed"
        }
    } elseif ($extension -eq '.json') {
        try { Get-Content -Raw -LiteralPath $fullPath | ConvertFrom-Json | Out-Null }
        catch { Fail "$($row.asset_id) is invalid JSON" }
    } elseif ($row.width -ne '-' -or $row.height -ne '-') {
        Fail "$($row.asset_id) non-image dimensions must be '-'"
    }
}

$unregistered = @(Get-ChildItem -LiteralPath $assetRoot -File -Recurse | ForEach-Object {
    [IO.Path]::GetRelativePath($Root, $_.FullName).Replace('\','/')
} | Where-Object { $_ -ne 'assets/README.md' -and -not $registeredPaths.Contains($_) })
if ($unregistered) { Fail "unregistered files: $($unregistered -join ', ')" }

Write-Output "Asset registry validation passed: $($rows.Count) governed assets"
