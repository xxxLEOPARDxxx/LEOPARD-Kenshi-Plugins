$ErrorActionPreference = 'Stop'
$base = Split-Path $PSScriptRoot -Parent
$out = 'D:/DEV/Kenshi/research/global-satellite-20261031/policy-tests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$vc = 'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk = 'C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE = "$vc/include;$sdk/Include"
$env:LIB = "$vc/lib/amd64;$sdk/Lib/x64"
$env:PATH = "$vc/bin/amd64;$env:PATH"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/policy_tests.exe" "/Fo$out/policy_tests.obj" "$base/tests/policy_tests.cpp"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$out/policy_tests.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/validation_tests.exe" "/Fo$out/" "$base/tests/validation_tests.cpp" "$base/CaptureValidation.cpp"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$out/validation_tests.exe"
exit $LASTEXITCODE
