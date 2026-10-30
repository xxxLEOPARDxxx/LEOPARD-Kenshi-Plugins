$ErrorActionPreference='Stop'
$module=Split-Path $PSScriptRoot -Parent
$out='D:/DEV/Kenshi/build/RecruitMarkersLeaderTests'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$out"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$env:PATH="$vc/bin/amd64;$env:PATH"
$source=[IO.File]::ReadAllText("$module/RecruitMarkers.cpp")
$begin=$source.IndexOf('bool HasLiveSquadFollower(')
if($begin -lt 0){$begin=$source.IndexOf('bool SafeFilter(')}
$end=$source.IndexOf('bool ByDistance(',$begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Production boundary not found'}
[IO.File]::WriteAllText("$out/leader_filter_under_test.h",$source.Substring($begin,$end-$begin))
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /DWIN32 /DNDEBUG "/Fo$out/" "/Fe$out/leader_filter.exe" "$PSScriptRoot/leader_filter.cpp"
if($LASTEXITCODE){exit $LASTEXITCODE}
& "$out/leader_filter.exe"
exit $LASTEXITCODE
