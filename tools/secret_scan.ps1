param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)

# The secret scan for the Genesis repository.
#
# It is a gate, not a warning: a credential that reaches a commit is
# published, and the history cannot be un-published, only rotated. So
# it fails the build when it finds something.
#
# Two things it has to get right, and both were wrong.
#
# First, it must scan what Git is TRACKING, not everything on the disk.
# A developer's local state is not the repository's problem; a committed
# key is. The scan used to walk the whole tree, so the local
# .genesis-local intake and the __pycache__ directories tripped it.
#
# Second, it needs an allowlist, and every entry in it has to carry a
# reason. An exclusion with no stated reason is not an exclusion, it is a
# hole. Without one, the scan failed on a test fixture of the scanner
# itself and on a placeholder in an .env.example, so the gate could not
# be run, and a gate that cannot be run certifies nothing.
#
# The accumulator is NOT called $matches. $Matches is a PowerShell
# automatic variable, and the -match operator overwrites it with a
# hashtable of the groups it just matched. Accumulating into it tried to
# add a string to a hashtable, and the scan died before it had scanned
# anything at all.

$ErrorActionPreference = "Stop"

$patterns = @(
    "-----BEGIN (RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----",
    "gh[pousr]_[A-Za-z0-9_]{20,}",
    "AKIA[0-9A-Z]{16}",
    '(?i)(password|passwd|secret|api[_-]?key|access[_-]?token)\s*[:=]\s*["''][^"''\r\n]{8,}["'']'
)

# Files that hold a credential SHAPE rather than a credential. Each one
# carries the reason it is here.
$allowlist = @{
    "integration-archive/selfcheck.py" = "A test fixture for the scanner itself: it passes the literal string 'api_key = ...' to check that the scanner detects one."
    "integration-archive/test_components.py" = "A test fixture for the scanner itself, in the same shape."
    "integration-archive/genesis_integration_01/genesis_runtime/selfcheck.py" = "A test fixture for the scanner itself, in the same shape."
    "integration-archive/genesis_integration_01/tests/test_components.py" = "A test fixture for the scanner itself, in the same shape."
    "integration-archive/genesis_runtime/selfcheck.py" = "A test fixture for the scanner itself, in the same shape."
    "integration-archive/tests/test_components.py" = "A test fixture for the scanner itself, in the same shape."
    "references/omniagent-runtime-&-workspace/.env.example" = "An example environment file. Its value is a placeholder for the owner to replace, not a key."
    "tools/secret_scan.ps1" = "This is the scanner. It holds the patterns it looks for."
}

$excluded = @("\.git", "[\\/]build([\\/]|$)", "[\\/]out([\\/]|$)", "[\\/]benchmark-results([\\/]|$)")

# The tracked tree, when the root is a repository. The fallback is the
# file walk, for a root that is not one.
$tracked = @()
Push-Location -LiteralPath $Root
try {
    $tracked = @(git ls-files -- 2>$null)
} finally {
    Pop-Location
}

if ($tracked.Count -gt 0) {
    $files = $tracked | ForEach-Object {
        $full = Join-Path $Root $_
        if (Test-Path -LiteralPath $full -PathType Leaf) { Get-Item -LiteralPath $full }
    }
} else {
    $files = Get-ChildItem -LiteralPath $Root -Recurse -File | Where-Object {
        $full = $_.FullName
        -not ($excluded | Where-Object { $full -match $_ }) -and $_.Length -lt 5MB
    }
}

$hits = @()
$scanned = 0
$skipped = 0
foreach ($file in $files) {
    if ($null -eq $file) { continue }
    $relative = $file.FullName.Substring($Root.Length).TrimStart('\', '/')
    $relative = $relative -replace '\\', '/'
    if ($excluded | Where-Object { $relative -match $_ }) { continue }
    if ($file.Length -ge 5MB) { continue }
    if ($allowlist.ContainsKey($relative)) {
        $skipped++
        continue
    }
    $content = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
    if ($null -eq $content) { continue }
    $scanned++
    foreach ($pattern in $patterns) {
        if ($content -match $pattern) {
            $hits += $relative
            break
        }
    }
}

if ($hits.Count -gt 0) {
    throw "Potential secret material found in: $($hits -join ', ')"
}
Write-Output "Secret scan passed: no high-confidence credential patterns found in $scanned file(s); $($skipped) file(s) allowed with a stated reason"
