$ErrorActionPreference='Stop'
$base=Split-Path $PSScriptRoot -Parent
$out=Join-Path $base 'build/quality-policy-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out/tests.obj" "/Fe$out/tests.exe" "$PSScriptRoot/atlas_quality_policy_tests.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){throw 'Quality policy compile failed'}
& "$out/tests.exe"
if($LASTEXITCODE -ne 0){throw 'Quality policy failed'}
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python "$PSScriptRoot/quality_policy_mutations.py"
if($LASTEXITCODE -ne 0){throw 'Quality policy functional negatives failed'}
