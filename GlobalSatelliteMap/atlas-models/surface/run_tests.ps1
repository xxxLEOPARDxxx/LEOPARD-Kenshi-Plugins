param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC';$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated';New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot";$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out/warp.obj" "/Fe$out/warp.exe" "$PSScriptRoot/warp_tests.cpp" /link d3d11.lib dxgi.lib
if($LASTEXITCODE -ne 0){throw 'Surface WARP compile failed'}
& "$out/warp.exe";if($LASTEXITCODE -ne 0){throw 'Surface WARP tests failed'}
$kl='D:/DEV/Kenshi/KenshiLib-src/Include';$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /W1 /DUNICODE /D_UNICODE "/Fo$out/adapter.obj" "$PSScriptRoot/compile_adapter.cpp" *> "$out/adapter-compile.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/adapter-compile.log" -Tail 8;throw 'Surface full SDK adapter compile failed'}
Write-Output 'Surface full SDK adapter compile PASS (not invoked)'
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/partial_init_tests.py";if($LASTEXITCODE -ne 0){throw 'Surface extracted Init/Clear tests failed'}
if(-not $SkipMutation){& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/mutation_tests.py";if($LASTEXITCODE -ne 0){throw 'Surface mutants failed'}}
