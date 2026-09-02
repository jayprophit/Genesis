param([string]$Source='docs/specifications/source/genesis2.txt',[string]$Output='registry/genesis2_sections.tsv')
$ErrorActionPreference='Stop'
$lines=Get-Content -LiteralPath $Source
$rows=[System.Collections.Generic.List[string]]::new()
$rows.Add("id`tline`theading`tsha256`tclassification`tstatus")
$seen=@{}
for($i=0;$i -lt $lines.Count;$i++) {
  if($lines[$i] -match '^#{1,6}\s+(.+?)\s*$') {
    $heading=$Matches[1] -replace "`t",' '
    $normalized=($heading.ToLowerInvariant() -replace '[^a-z0-9]+','-').Trim('-')
    if(!$seen.ContainsKey($normalized)){$seen[$normalized]=0};$seen[$normalized]++
    $material="$($i+1):$heading"
    $bytes=[Text.Encoding]::UTF8.GetBytes($material)
    $hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
    $class=if($heading -match 'canonical|requirement|genuinely new'){'candidate-requirement'}elseif($heading -match 'repo|video|tool|useful|copy'){'research-evaluation'}else{'design-discussion'}
    $id='G2-{0:D4}' -f $rows.Count
    $rows.Add(($id,($i+1),$heading,$hash,$class,'DISCOVERED' -join "`t"))
  }
}
$rows | Set-Content -LiteralPath $Output -Encoding utf8NoBOM
Write-Output "Indexed $($rows.Count-1) genesis2 headings; repeated headings remain independently traceable by line."
