[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$files=Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/fdrive_research_files.tsv')
$links=Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/fdrive_links.tsv')
$verified=Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/fdrive_verified_links.tsv')
if($files.Count -eq 0 -or $links.Count -eq 0){throw 'F-drive inventories must not be empty'}
$paths=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($file in $files){
 if(!$paths.Add($file.path)){throw "Duplicate inventory path: $($file.path)"}
 if($file.bytes -notmatch '^\d+$'){throw "Invalid byte count: $($file.path)"}
 if($file.sha256 -ne 'READ_ERROR_CRC' -and $file.sha256 -notmatch '^[0-9a-f]{64}$'){throw "Invalid hash: $($file.path)"}
 if($file.decision -notin @('READ_ONLY_RESEARCH','UNREADABLE_CRC')){throw "Invalid decision: $($file.path)"}
}
$urls=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($link in $links){if(!$urls.Add($link.url)){throw "Duplicate link: $($link.url)"};if($link.verification -ne 'UNVERIFIED'){throw "Raw link registry must default to UNVERIFIED: $($link.url)"}}
foreach($link in $verified){if($link.status -notin @('VERIFIED','VERIFIED_RESEARCH','ARCHIVED')){throw "Invalid verified-link status: $($link.url)"}}
$crc=@($files|Where-Object decision -eq 'UNREADABLE_CRC').Count
Write-Output "F-drive inventory validation passed: $($files.Count) files, $($links.Count) links, $($verified.Count) priority verifications, $crc unreadable CRC file."
