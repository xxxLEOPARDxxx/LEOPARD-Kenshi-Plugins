# Local LoK update, staged and reviewed; default is validation only.
# Run ONLY after the parent completes the isolated KEO trace.
[CmdletBinding()]
param([switch]$Install)
$ErrorActionPreference = 'Stop'
$taskRoot = 'D:\SteamLibrary\steamapps\common\Kenshi\_codex_work\20261105-lok-update'
$gameRoot = 'D:\SteamLibrary\steamapps\common\Kenshi'
$installedRoot = Join-Path $gameRoot 'mods\Legendary of Kenshi'
$stageRoot = Join-Path $taskRoot 'prepared\Legendary of Kenshi'
$backupRoot = 'D:\DEV\Kenshi\backup\LoK before update 2026-11-05'
function Hash([string]$p) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() }
function AssertPlain([string]$p,[string]$root) {
 $absolute = [IO.Path]::GetFullPath($p); $limit = [IO.Path]::GetFullPath($root).TrimEnd('\')
 if ($absolute -ne $limit -and -not $absolute.StartsWith($limit+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Path outside permitted root: $p" }
 $cursor = $absolute
 while ($true) {
  $node = Get-Item -LiteralPath $cursor -Force
  if (($node.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse path refused: $cursor" }
  if ($cursor.Equals($limit,[StringComparison]::OrdinalIgnoreCase)) { break }
  $cursor = [IO.Path]::GetDirectoryName($cursor)
 }
}
function AssertClosed {
 if (@(Get-Process -Name '*kenshi*' -ErrorAction SilentlyContinue).Count) { throw 'Kenshi is running; no files replaced.' }
}
$reportPath = Join-Path $taskRoot 'verification-20261105.json'
$manifestPath = Join-Path $taskRoot 'installed-files.json'
if ((Hash $reportPath) -ne '9429349c76894841e2247038581b183e3ebdaa878e1aef116841d3ceb05d0823' -or (Hash $manifestPath) -ne '6cf9b6b837b8ad6c215e6baefce30f09e37a77fc902a3e3295c1f7e0e4d07f78') { throw 'Pinned verification inputs changed.' }
$report = Get-Content -LiteralPath $reportPath -Raw -Encoding UTF8 | ConvertFrom-Json
$manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$rows = @($report.staged_files)
if ($rows.Count -ne 3 -or @($manifest.PSObject.Properties).Count -ne 241) { throw 'Unexpected manifest size.' }
$allowed = @('Legendary of Kenshi.mod','_Legendary of Kenshi.info','lok_npcs.dll')
foreach ($row in $rows) {
 if ($allowed -notcontains $row.path) { throw 'Unexpected replacement path.' }
 foreach ($pair in @(@($stageRoot,$row.prepared_sha256),@($backupRoot,$row.before_sha256))) {
  $p = Join-Path $pair[0] $row.path; AssertPlain $p $pair[0]
  if ((Hash $p) -ne $pair[1]) { throw "Staged/backup hash mismatch: $p" }
 }
}
# Require every prior file unchanged; newly created unrelated files are preserved.
foreach ($property in $manifest.PSObject.Properties) {
 if ([IO.Path]::IsPathRooted($property.Name) -or $property.Name.Split('/') -contains '..') { throw 'Unsafe relative manifest path.' }
 $p = Join-Path $installedRoot $property.Name; AssertPlain $p $installedRoot
 if ((Hash $p) -ne $property.Value) { throw "Installed baseline changed: $p" }
}
$protected = @('data\mods.cfg','mods\Legendary of Kenshi RU\Legendary of Kenshi RU.mod','mods\THE_NCE_Patch\THE_NCE_Patch.mod','mods\Legendary of Kenshi All Anim patch\Legendary of Kenshi All Anim patch.mod','mods\Legendary of Kenshi All Robots Module Patch\Legendary of Kenshi All Robots Module Patch.mod')
$protectedBefore = @{}
foreach ($relative in $protected) { $p=Join-Path $gameRoot $relative; AssertPlain $p $gameRoot; $protectedBefore[$relative]=Hash $p }
if (-not $Install) { Write-Output 'VALIDATED: three staged files; 241 installed baseline files unchanged; RU/NCE/patches and current mod list protected. No install performed.'; return }
AssertClosed
$temps = @(); $touched = @(); $complete = $false
try {
 foreach ($row in $rows) {
  $tmp = Join-Path $installedRoot ($row.path+'.lok-update-20261105.tmp')
  if (Test-Path -LiteralPath $tmp) { throw "Temporary path exists: $tmp" }
  Copy-Item -LiteralPath (Join-Path $stageRoot $row.path) -Destination $tmp
  $temps += $tmp
  if ((Hash $tmp) -ne $row.prepared_sha256) { throw 'Temporary file hash mismatch.' }
 }
 foreach ($row in $rows) {
  AssertClosed
  $destination=Join-Path $installedRoot $row.path; AssertPlain $destination $installedRoot
  if ((Hash $destination) -ne $row.before_sha256) { throw 'Installed file changed during staging.' }
  $touched += $row
  Copy-Item -LiteralPath ($destination+'.lok-update-20261105.tmp') -Destination $destination -Force
  if ((Hash $destination) -ne $row.prepared_sha256) { throw 'Installed replacement hash mismatch.' }
 }
 foreach ($property in $manifest.PSObject.Properties) {
  $expected=$property.Value
  foreach ($row in $rows) { if ($row.path -eq $property.Name) { $expected=$row.prepared_sha256 } }
  if ((Hash (Join-Path $installedRoot $property.Name)) -ne $expected) { throw 'Final baseline verification failed.' }
 }
 foreach ($relative in $protected) { if ((Hash (Join-Path $gameRoot $relative)) -ne $protectedBefore[$relative]) { throw "Protected file changed: $relative" } }
 $receipt=[ordered]@{status='installed_locally';time=(Get-Date -Format o);files=$rows;protected_hashes=$protectedBefore;other_baseline_files_unchanged=238;backup=$backupRoot}
 $receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $taskRoot 'installation-20261105.json') -Encoding UTF8
 $complete=$true
 Write-Output 'INSTALLED LOCALLY: exactly three verified LoK original files; translation/NCE/patches/mod list unchanged. No publication.'
}
catch {
 $originalError=$_
 $rollbackErrors=@()
 foreach ($row in $touched) {
  try { $destination=Join-Path $installedRoot $row.path; Copy-Item -LiteralPath (Join-Path $backupRoot $row.path) -Destination $destination -Force; if ((Hash $destination) -ne $row.before_sha256) { throw 'Rollback hash mismatch.' } }
  catch { $rollbackErrors += $_.Exception.Message }
 }
 if ($rollbackErrors.Count) { throw ('INSTALL FAILED; rollback incomplete: '+($rollbackErrors -join '; ')+'; original: '+$originalError.Exception.Message) }
 throw ('INSTALL FAILED; touched files restored byte-exact: '+$originalError.Exception.Message)
}
finally {
 foreach ($tmp in $temps) { if (Test-Path -LiteralPath $tmp) { AssertPlain $tmp $installedRoot; Remove-Item -LiteralPath $tmp -Force } }
}
