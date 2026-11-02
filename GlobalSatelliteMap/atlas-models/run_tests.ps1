param([switch]$StateOnly,[switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /W4 /O2 /MT "$PSScriptRoot/state_tests.cpp" "/Fe$out/state_tests.exe" "/Fo$out/state_tests.obj" /link d3d11.lib dxgi.lib
if($LASTEXITCODE -ne 0){throw 'Model graphics test build failed'}
& "$out/state_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Model graphics-state test failed'}
if($StateOnly){exit 0}
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
& $python "$PSScriptRoot/generate_models.py"
if($LASTEXITCODE -ne 0){throw 'Pinned native model generation failed'}
& $python "$PSScriptRoot/verify_contract.py"
if($LASTEXITCODE -ne 0){throw 'Actual model shader/native source contract failed'}
& $fxc /nologo /T gs_5_0 /E main_gs /O3 "/Fo$out/ModelStateGS.cso" "$PSScriptRoot/StateSentinels.hlsl"
if($LASTEXITCODE -ne 0){throw 'GS state fixture failed'}
& $fxc /nologo /T ps_5_0 /E main_ps /O3 "/Fo$out/ModelStatePS.cso" "$PSScriptRoot/StateSentinels.hlsl"
if($LASTEXITCODE -ne 0){throw 'Dynamic class state fixture failed'}
foreach($case in @('policy','admission','warp')){
 & "$vc/bin/amd64/cl.exe" /nologo /EHsc /W4 /O2 /MT "$PSScriptRoot/${case}_tests.cpp" "/Fe$out/${case}_tests.exe" "/Fo$out/${case}_tests.obj" /link d3d11.lib dxgi.lib
 if($LASTEXITCODE -ne 0){throw "$case build failed"}
 Push-Location $out
 try {& "$out/${case}_tests.exe";if($LASTEXITCODE -ne 0){throw "$case tests failed"}} finally {Pop-Location}
}
$kl='D:/DEV/Kenshi/deps/KenshiLib/Include'
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "$PSScriptRoot/native_compile.cpp" "/Fo$out/native_compile.obj"
if($LASTEXITCODE -ne 0){throw 'Actual native ABI adapter compile failed'}
if(-not $SkipMutation){& $python "$PSScriptRoot/test_mutants.py";if($LASTEXITCODE -ne 0){throw 'Model functional mutants failed'}}
