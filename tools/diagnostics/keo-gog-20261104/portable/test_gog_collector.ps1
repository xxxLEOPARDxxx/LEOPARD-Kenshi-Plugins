param([string]$ScriptPath = (Join-Path $PSScriptRoot 'Collect-GOG-Profile.ps1'))
$ErrorActionPreference = 'Stop'
$parseErrors=$null; $tokens=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($ScriptPath,[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw 'PowerShell parse errors' }
$functions=@($ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]},$false))
foreach ($function in $functions) { . ([scriptblock]::Create($function.Extent.Text)) }
$checks=0
function Check([bool]$Value,[string]$Why) { $script:checks++; if (-not $Value) { throw $Why } }
function Refuses([scriptblock]$Action,[string]$Why) { $refused=$false;try { & $Action | Out-Null } catch { $refused=$true }; Check $refused $Why }
function Put16([byte[]]$B,[int]$At,[UInt16]$Value) { [BitConverter]::GetBytes($Value).CopyTo($B,$At) }
function Put32([byte[]]$B,[int]$At,[UInt32]$Value) { [BitConverter]::GetBytes($Value).CopyTo($B,$At) }
$b=New-Object byte[] 1024
$b[0]=77;$b[1]=90;Put32 $b 60 128;Put32 $b 128 17744
Put16 $b 132 34404;Put16 $b 134 1;Put16 $b 148 240;Put16 $b 152 523
Put32 $b 168 4096;Put32 $b 208 8192;Put32 $b 212 512
[Text.Encoding]::ASCII.GetBytes('.text').CopyTo($b,392)
Put32 $b 400 256;Put32 $b 404 4096;Put32 $b 408 512;Put32 $b 412 512
for ($i=0;$i -lt 32;$i++) { $b[512+$i]=[byte]$i }
$p=ReadPE $b
Check ($p.machine -eq 34404 -and $p.sections.Count -eq 1) 'valid PE64 rejected'
Check ((WindowHex $b $p 4096) -eq '000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f') 'wrong actual byte window'
Refuses { WindowHex $b $p 4592 } 'cross-section window accepted'
Refuses { WindowHex $b $p -1 } 'negative RVA accepted'
Refuses { ReadPE ([byte[]]@(77,90)) } 'short header accepted'
$c=[byte[]]$b.Clone();Put16 $c 132 332;Refuses { ReadPE $c } 'x86 accepted'
$c=[byte[]]$b.Clone();Put16 $c 134 97;Refuses { ReadPE $c } 'unbounded sections accepted'
$c=[byte[]]$b.Clone();Put32 $c 412 4294967280;Refuses { ReadPE $c } 'section outside file accepted'
Check ((HashBytes ([byte[]]@(97,98,99)) 'MD5') -eq '900150983cd24fb0d6963f7d28e17f72') 'MD5 mismatch'
Check ((HashBytes ([byte[]]@(97,98,99)) 'SHA256') -eq 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad') 'SHA256 mismatch'
Write-Output "PASS $checks actual PowerShell collector boundary/hash checks"
