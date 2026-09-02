param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$patterns = @(
    "-----BEGIN (RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----",
    "gh[pousr]_[A-Za-z0-9_]{20,}",
    "AKIA[0-9A-Z]{16}",
    '(?i)(password|passwd|secret|api[_-]?key|access[_-]?token)\s*[:=]\s*["''][^"''\r\n]{8,}["'']'
)
$excluded = @("\.git", "[\\/]build([\\/]|$)", "[\\/]out([\\/]|$)", "[\\/]benchmark-results([\\/]|$)")
$files = Get-ChildItem -LiteralPath $Root -Recurse -File | Where-Object {
    $full = $_.FullName
    -not ($excluded | Where-Object { $full -match $_ }) -and $_.Length -lt 5MB
}
$matches = @()
foreach ($file in $files) {
    $content = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
    foreach ($pattern in $patterns) {
        if ($content -match $pattern) {
            $matches += $file.FullName
            break
        }
    }
}
if ($matches.Count -gt 0) {
    throw "Potential secret material found in: $($matches -join ', ')"
}
Write-Output "Secret scan passed: no high-confidence credential patterns found"
