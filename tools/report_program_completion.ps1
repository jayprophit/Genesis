[CmdletBinding()]
param([string]$Output='registry/completion_report.tsv')
$ErrorActionPreference='Stop';$repo=Split-Path -Parent $PSScriptRoot
$rows=Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $repo 'registry/completion_components.tsv')
$weights=[ordered]@{design=10;implementation=25;unit_tests=15;integration_tests=15;benchmark=10;security=10;recovery=5;documentation=5;platform_qualification=5}
$byId=@{};foreach($r in $rows){if($byId.ContainsKey($r.id)){throw "Duplicate component $($r.id)"};$byId[$r.id]=$r}
foreach($r in $rows){if($r.parent -ne '-' -and !$byId.ContainsKey($r.parent)){throw "Unknown parent $($r.parent)"}}
$scores=@{}
function Score([string]$id){if($scores.ContainsKey($id)){return $scores[$id]};$children=@($rows|Where-Object parent -eq $id);if($children.Count){$value=($children|ForEach-Object {Score $_.id}|Measure-Object -Average).Average}else{$r=$byId[$id];$value=0;foreach($g in $weights.Keys){if($r.$g -notin @('0','1')){throw "Invalid gate $g for $id"};$value+=[int]$r.$g*$weights[$g]}};$scores[$id]=[math]::Round($value,2);return $scores[$id]}
$out=[Collections.Generic.List[string]]::new();$out.Add("id`tparent`tname`tpercentage`tstatus`trequirements")
foreach($r in $rows){$p=Score $r.id;$status=if($p -eq 100){'COMPLETE'}elseif($p -eq 0){'NOT_STARTED'}else{'IN_PROGRESS'};$out.Add(($r.id,$r.parent,$r.name,$p,$status,$r.requirements -join "`t"))}
$target=Join-Path $repo $Output
[IO.File]::WriteAllLines($target,$out,[Text.UTF8Encoding]::new($false))
foreach($r in $rows|Where-Object parent -eq 'GENESIS'){"{0}: {1}%" -f $r.name,(Score $r.id)}
"Genesis total: $(Score 'GENESIS')%"
