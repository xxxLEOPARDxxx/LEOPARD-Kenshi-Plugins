param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
& $python "$PSScriptRoot/test_sources.py"
if($LASTEXITCODE -ne 0){throw 'Native source tests failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 /wd4127 "/Fo$out/policy.obj" "/Fe$out/policy.exe" "$PSScriptRoot/policy_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Policy compile failed'}
& "$out/policy.exe"
if($LASTEXITCODE -ne 0){throw 'Policy failed'}
& $fxc /nologo /T cs_5_0 /E main /O3 /Gec "/Fo$out/AtlasWater.cso" "$PSScriptRoot/AtlasWater.hlsl"
if($LASTEXITCODE -ne 0){throw 'Water compute compile failed'}
& $fxc /nologo /T ps_5_0 /E waterFP /O3 /Gec /D DISTANT=1 /D NO_REFLECTION=1 /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' "/Fo$out/NativeWater.cso" 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/forward/water.hlsl'
if($LASTEXITCODE -ne 0){throw 'Actual native water PS failed'}
& $fxc /nologo /T vs_5_0 /E main /O3 /Gec "/Fo$out/ReferenceVS.cso" "$PSScriptRoot/ReferenceVS.hlsl"
if($LASTEXITCODE -ne 0){throw 'Reference VS failed'}
& $fxc /nologo /T ps_5_0 /E main /O3 /Gec "/Fo$out/ReferenceTerrainPS.cso" "$PSScriptRoot/ReferenceTerrainPS.hlsl"
if($LASTEXITCODE -ne 0){throw 'Reference terrain PS failed'}
& $python "$PSScriptRoot/verify_shaders.py"
if($LASTEXITCODE -ne 0){throw 'Compiled water ABI failed'}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$out/warp.obj" "/Fe$out/warp.exe" "$PSScriptRoot/warp_tests.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){throw 'WARP compile failed'}
& "$out/warp.exe" "$out/AtlasWater.cso" "$out/NativeWater.cso" "$out/ReferenceVS.cso" "$out/ReferenceTerrainPS.cso"
if($LASTEXITCODE -ne 0){throw 'Water WARP failed'}
if(-not $SkipMutation){
 & $python "$PSScriptRoot/test_mutations.py"
 if($LASTEXITCODE -ne 0){throw 'Water negative controls failed'}
}
