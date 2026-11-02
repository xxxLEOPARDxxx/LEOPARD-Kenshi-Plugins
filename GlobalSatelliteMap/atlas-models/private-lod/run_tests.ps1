param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC';$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1';$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$out=Join-Path $PSScriptRoot '_generated';New-Item -ItemType Directory -Force $out | Out-Null
& $python "$PSScriptRoot/generate_profile.py";if($LASTEXITCODE -ne 0){throw 'Private native profile audit failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot";$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out/policy.obj" "/Fe$out/policy.exe" "$PSScriptRoot/policy_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Private asset/policy compile failed'}
& "$out/policy.exe" 'D:/SteamLibrary/steamapps/common/Kenshi/data/meshes/distant';if($LASTEXITCODE -ne 0){throw 'Private asset/policy tests failed'}
& $python "$PSScriptRoot/test_bounds.py";if($LASTEXITCODE -ne 0){throw 'Private native bounds value tests failed'}
& $python "$PSScriptRoot/extract_control.py";if($LASTEXITCODE -ne 0){throw 'Private control extraction failed'}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out/control.obj" "/Fe$out/control.exe" "$PSScriptRoot/control_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Private extracted native control compile failed'}
& "$out/control.exe";if($LASTEXITCODE -ne 0){throw 'Private extracted native controls failed'}
$kl='D:/DEV/Kenshi/KenshiLib-src/Include';$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W1 /DUNICODE /D_UNICODE /DBOOST_ALL_NO_LIB "/Fo$out/abi.obj" "/Fe$out/abi.exe" "$PSScriptRoot/abi_tests.cpp" 'D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib/libboost_system-vc100-mt-1_60.lib' *> "$out/abi-build.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/abi-build.log" -Tail 8;throw 'Serializer/stream SDK ABI test compile failed'}
& "$out/abi.exe";if($LASTEXITCODE -ne 0){throw 'Serializer/stream SDK ABI test failed'}
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /W1 /DUNICODE /D_UNICODE "/Fo$out/adapter.obj" "$PSScriptRoot/compile_adapter.cpp" *> "$out/adapter-build.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/adapter-build.log" -Tail 8;throw 'Complete private native adapter compile failed'}
Write-Output 'Complete private native adapter VC10 compile PASS (not linked/invoked)'
if(-not $SkipMutation){& $python "$PSScriptRoot/mutation_tests.py";if($LASTEXITCODE -ne 0){throw 'Private functional mutants failed'}}
