param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
& $python "$PSScriptRoot/generate_profile.py"
if($LASTEXITCODE -ne 0){throw 'Loaded census native profile failed'}
& $python "$PSScriptRoot/verify_contract.py"
if($LASTEXITCODE -ne 0){throw 'Loaded census source contract failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /W4 /O2 /MT "$PSScriptRoot/route_tests.cpp" "/Fe$out/route.exe" "/Fo$out/route.obj"
if($LASTEXITCODE -ne 0){throw 'Loaded census route build failed'}
& "$out/route.exe"
if($LASTEXITCODE -ne 0){throw 'Loaded census route tests failed'}
& $python "$PSScriptRoot/test_native_extracted.py"
if($LASTEXITCODE -ne 0){throw 'Loaded census actual native extracts failed'}
$kl='D:/DEV/Kenshi/deps/KenshiLib/Include'
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "$PSScriptRoot/native_compile.cpp" "/Fo$out/native.obj"
if($LASTEXITCODE -ne 0){throw 'Loaded census actual native ABI compile failed'}
if(-not $SkipMutation){& $python "$PSScriptRoot/test_mutants.py";if($LASTEXITCODE -ne 0){throw 'Loaded census functional mutants failed'}}
