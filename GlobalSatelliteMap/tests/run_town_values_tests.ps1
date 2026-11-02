$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/town-values-review'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /GL /W3 "/Fe$out/tests.exe" "/Fo$out/tests.obj" "$PSScriptRoot/town_values_tests.cpp" /link /LTCG
if($LASTEXITCODE -ne 0){throw 'Town values compile failed'}
& "$out/tests.exe"
if($LASTEXITCODE -ne 0){throw 'Town values test failed'}
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/town_values_contract.py"
if($LASTEXITCODE -ne 0){throw 'Town values contract/mutants failed'}
$kl='D:/DEV/Kenshi/deps/KenshiLib/Include'
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "/Fo$out/native_compile.obj" "$PSScriptRoot/town_values_native_compile.cpp" *> "$out/native_compile.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/native_compile.log" -Tail 15;throw 'Town values full SDK compile failed'}
'Town values full SDK VC10/O2/GL compile PASS (not linked/invoked)'
