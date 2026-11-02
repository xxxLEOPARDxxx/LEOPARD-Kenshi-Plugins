$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/town-diagnostic-review'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/tests.exe" "/Fo$out/tests.obj" "$PSScriptRoot/town_diagnostic_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Town census compile failed'}
& "$out/tests.exe"
if($LASTEXITCODE -ne 0){throw 'Town census test failed'}
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/town_diagnostic_contract.py"
if($LASTEXITCODE -ne 0){throw 'Town native contract/mutants failed'}
