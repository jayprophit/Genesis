[CmdletBinding()]
param([string]$OutputDirectory='registry')
$ErrorActionPreference='Stop'
$roots=@(
 'F:\ai chat conversations\chatgpt',
 'F:\ai chat conversations\claude',
 'F:\ai chat conversations\deepseek',
 'F:\ai chat conversations\other',
 'F:\AI Digital Twin'
)
$exclude='[\\/](node_modules|\.git|build|dist|\.venv|venv|target|\.cache)[\\/]'
$files=[System.Collections.Generic.List[object]]::new()
$links=[System.Collections.Generic.Dictionary[string,object]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($root in $roots){
 if(!(Test-Path -LiteralPath $root)){continue}
 $paths=& rg --files $root -g '*.txt' -g '*.md' -g '!**/node_modules/**' -g '!**/.git/**' -g '!**/build/**' -g '!**/dist/**' -g '!**/.venv/**' -g '!**/venv/**' -g '!**/target/**' -g '!**/.cache/**'
 foreach($path in $paths){
  if($path -match $exclude){continue}
  $item=Get-Item -LiteralPath $path
  try {$hash=(Get-FileHash -Algorithm SHA256 -LiteralPath $path -ErrorAction Stop).Hash.ToLowerInvariant()} catch {$hash='READ_ERROR_CRC'}
  try {$text=Get-Content -Raw -LiteralPath $path -ErrorAction Stop} catch {$text=''}
  if($null -eq $text){$text=''}
  $urlMatches=[regex]::Matches($text,'https?://[^\s<>"''\]\)]+')
  foreach($match in $urlMatches){
   $url=$match.Value.TrimEnd('.',',',';',':')
   if(!$links.ContainsKey($url)){$links[$url]=[pscustomobject]@{url=$url;occurrences=0;source_count=0;sources=[System.Collections.Generic.HashSet[string]]::new()}}
   $entry=$links[$url];$entry.occurrences++
   if($entry.sources.Add($path)){$entry.source_count++}
  }
  $topics=[System.Collections.Generic.List[string]]::new()
  $map=[ordered]@{
   memory='memory|remember|forget|retriev|consolidat'; cognition='cognit|reason|attention|world model|self model'; learning='learn|training|gradient|optimizer';
   development='embry|teach|curricul|sleep|play|matur|independen'; multimodal='multimodal|vision|audio|speech|language|avatar|robot|sensor';
   networking='network|distributed|protocol|sync|shared domain|agent'; security='security|crypt|key custody|authentic|fuzz|sanitizer';
   evolution='descendant|population|evolution|fitness|mutation'; provenance='provenance|license|source|evidence'; code='c\+\+|cmake|python|typescript|implementation|algorithm'
  }
  foreach($pair in $map.GetEnumerator()){if($text -match $pair.Value){$topics.Add($pair.Key)}}
  $decision=if($hash -eq 'READ_ERROR_CRC'){'UNREADABLE_CRC'}else{'READ_ONLY_RESEARCH'}
  $files.Add([pscustomobject]@{path=$path;bytes=$item.Length;sha256=$hash;topics=($topics -join ',');links=$urlMatches.Count;decision=$decision})
 }
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$fileLines=[System.Collections.Generic.List[string]]::new();$fileLines.Add("path`tbytes`tsha256`ttopics`tlinks`tdecision")
foreach($f in ($files|Sort-Object path)){$fileLines.Add(($f.path,$f.bytes,$f.sha256,$f.topics,$f.links,$f.decision -join "`t"))}
$fileLines|Set-Content -LiteralPath (Join-Path $OutputDirectory 'fdrive_research_files.tsv') -Encoding utf8NoBOM
$linkLines=[System.Collections.Generic.List[string]]::new();$linkLines.Add("url`thost`toccurrences`tsource_count`tclassification`tverification")
foreach($entry in ($links.Values|Sort-Object url)){
 try {$uri=[Uri]$entry.url;$linkHost=$uri.Host.ToLowerInvariant()} catch {$linkHost='invalid-template'}
 $class=if($linkHost -match 'github\.com|gitlab\.com'){'source-repository'}elseif($linkHost -match 'arxiv\.org|doi\.org|nature\.com|science\.org'){'research'}elseif($linkHost -match 'docs\.|developer\.|learn\.|w3\.org|ietf\.org'){'documentation'}else{'external-reference'}
 $linkLines.Add(($entry.url,$linkHost,$entry.occurrences,$entry.source_count,$class,'UNVERIFIED' -join "`t"))
}
$linkLines|Set-Content -LiteralPath (Join-Path $OutputDirectory 'fdrive_links.tsv') -Encoding utf8NoBOM
$duplicates=@($files|Group-Object sha256|Where-Object Count -gt 1)
Write-Output "Inventoried $($files.Count) files, $($links.Count) unique links, and $($duplicates.Count) duplicate-content hash groups."
