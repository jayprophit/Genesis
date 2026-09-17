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
    return $extension -in @('.cpp','.hpp','.md','.ps1','.json','.tsv','.yml','.yaml','.cmake','.svg') -or
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
    # Framework hasher for ALL inputs (fix 2026-09-16, TEST_DEFECT classification):
    # the previous hand-rolled SHA-256 used PowerShell arithmetic shifts (-shr
    # sign-extends) and -bnot (negative intermediates), throwing
    # "Cannot convert value ... to UInt32" on content that sets high bits and
    # risking wrong digests where it did not throw. SHA256CryptoServiceProvider
    # exists on both Windows PowerShell 5.1 and pwsh 7, so no compat shim is needed.
    if (-not $Bytes) { $Bytes = [byte[]]@() }
    $hasher = [System.Security.Cryptography.SHA256CryptoServiceProvider]::new()
    try {
        return ([BitConverter]::ToString($hasher.ComputeHash($Bytes)).Replace('-', '')).ToUpper()
    } finally {
        $hasher.Dispose()
    }
}

# --- Retired hand-rolled SHA-256 (kept out of execution path; see Sha256) ---
function Sha256_Retired_Manual([byte[]]$Bytes) {
    $K = @(
        [uint32]0x428a2f98, [uint32]0x71374491, [uint32]0xb5c0fbcf, [uint32]0xe9b5dba5, [uint32]0x3956c25b, [uint32]0x59f111f1, [uint32]0x923f82a4, [uint32]0xab1c5ed5,
        [uint32]0xd807aa98, [uint32]0x12835b01, [uint32]0x243185be, [uint32]0x550c7dc3, [uint32]0x72be5d74, [uint32]0x80deb1fe, [uint32]0x9bdc06a7, [uint32]0xc19bf174,
        [uint32]0xe49b69c1, [uint32]0xefbe4786, [uint32]0x0fc19dc6, [uint32]0x240ca1cc, [uint32]0x2de92c6f, [uint32]0x4a7484aa, [uint32]0x5cb0a9dc, [uint32]0x76f988da,
        [uint32]0x983e5152, [uint32]0xa831c66d, [uint32]0xb00327c8, [uint32]0xbf597fc7, [uint32]0xc6e00bf3, [uint32]0xd5a79147, [uint32]0x06ca6351, [uint32]0x14292967,
        [uint32]0x27b70a85, [uint32]0x2e1b2138, [uint32]0x4d2c6dfc, [uint32]0x53380d13, [uint32]0x650a7354, [uint32]0x766a0abb, [uint32]0x81c2c92e, [uint32]0x92722c85,
        [uint32]0xa2bfe8a1, [uint32]0xa81a664b, [uint32]0xc24b8b70, [uint32]0xc76c51a3, [uint32]0xd192e819, [uint32]0xd6990624, [uint32]0xf40e3585, [uint32]0x106aa070,
        [uint32]0x19a4c116, [uint32]0x1e376c08, [uint32]0x2748774c, [uint32]0x34b0bcb5, [uint32]0x391c0cb3, [uint32]0x4ed8aa4a, [uint32]0x5b9cca4f, [uint32]0x682e6ff3,
        [uint32]0x748f82ee, [uint32]0x78a5636f, [uint32]0x84c87814, [uint32]0x8cc70208, [uint32]0x90befffa, [uint32]0xa4506ceb, [uint32]0xbef9a3f7, [uint32]0xc67178f2
    )
    
    # Pre-processing
    $bitLen = $Bytes.Length * 8
    $padLen = (56 - ($Bytes.Length + 1) % 64) % 64
    $padded = [byte[]]::new($Bytes.Length + 1 + $padLen + 8)
    [Array]::Copy($Bytes, 0, $padded, 0, $Bytes.Length)
    $padded[$Bytes.Length] = 0x80
    # Append bit length as big-endian 64-bit integer
    for ($i = 7; $i -ge 0; $i--) {
        $padded[$padded.Length - 8 + (7 - $i)] = [byte](($bitLen -shr ($i * 8)) -band 0xFF)
    }
    
    # Initialize hash values
    $H = @(
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    )
    
    # Process each 512-bit chunk
    for ($chunkOffset = 0; $chunkOffset -lt $padded.Length; $chunkOffset += 64) {
        # Prepare message schedule
        $W = [uint32[]]::new(64)
        for ($i = 0; $i -lt 16; $i++) {
            $offset = $chunkOffset + $i * 4
            $W[$i] = ($padded[$offset] -shl 24) -bor ($padded[$offset + 1] -shl 16) -bor ($padded[$offset + 2] -shl 8) -bor $padded[$offset + 3]
        }
        for ($i = 16; $i -lt 64; $i++) {
            $s0 = (($W[$i - 15] -shr 7) -bor ($W[$i - 15] -shl 25)) -bxor (($W[$i - 15] -shr 18) -bor ($W[$i - 15] -shl 14)) -bxor ($W[$i - 15] -shr 3)
            $s1 = (($W[$i - 2] -shr 17) -bor ($W[$i - 2] -shl 15)) -bxor (($W[$i - 2] -shr 19) -bor ($W[$i - 2] -shl 13)) -bxor ($W[$i - 2] -shr 10)
            $W[$i] = ($W[$i - 16] + $s0 + $W[$i - 7] + $s1) -band 0xFFFFFFFF
        }
        
        # Initialize working variables
        $a = $H[0]; $b = $H[1]; $c = $H[2]; $d = $H[3]
        $e = $H[4]; $f = $H[5]; $g = $H[6]; $h = $H[7]
        
        # Main loop
        for ($i = 0; $i -lt 64; $i++) {
            $S1 = (($e -shr 6) -bor ($e -shl 26)) -bxor (($e -shr 11) -bor ($e -shl 21)) -bxor (($e -shr 25) -bor ($e -shl 7))
            $ch = ($e -band $f) -bxor ((-bnot $e) -band $g)
            $temp1 = (($h + $S1 + $ch + $K[$i] + $W[$i]) -band 0xFFFFFFFF)
            $S0 = (($a -shr 2) -bor ($a -shl 30)) -bxor (($a -shr 13) -bor ($a -shl 19)) -bxor (($a -shr 22) -bor ($a -shl 10))
            $maj = ($a -band $b) -bxor ($a -band $c) -bxor ($b -band $c)
            $temp2 = (($S0 + $maj) -band 0xFFFFFFFF)
            
            $h = $g; $g = $f; $f = $e; $e = ($d + $temp1) -band 0xFFFFFFFF
            $d = $c; $c = $b; $b = $a; $a = ($temp1 + $temp2) -band 0xFFFFFFFF
        }
        
        $H[0] = ($H[0] + $a) -band 0xFFFFFFFF
        $H[1] = ($H[1] + $b) -band 0xFFFFFFFF
        $H[2] = ($H[2] + $c) -band 0xFFFFFFFF
        $H[3] = ($H[3] + $d) -band 0xFFFFFFFF
        $H[4] = ($H[4] + $e) -band 0xFFFFFFFF
        $H[5] = ($H[5] + $f) -band 0xFFFFFFFF
        $H[6] = ($H[6] + $g) -band 0xFFFFFFFF
        $H[7] = ($H[7] + $h) -band 0xFFFFFFFF
    }
    
    # Convert to hex string
    $result = ""
    foreach ($val in $H) {
        $result += $val.ToString('X8')
    }
    return $result
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
    $absolute = $item.FullName
    $relative = $absolute.Substring($rootFull.Length + 1).Replace('\','/')
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
        bytes = $hashBytes.Length
        lines = if ($mediaClass -eq 'text') { Line-Count $hashBytes } else { 0 }
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
