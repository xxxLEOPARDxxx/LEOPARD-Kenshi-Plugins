$ErrorActionPreference = 'Stop'
$durabilityOut = 'D:/DEV/Kenshi/build/ExtraDurability-tests'
New-Item -ItemType Directory -Force -Path $durabilityOut | Out-Null
$durabilityVc = 'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$durabilitySdk = 'C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE = "$durabilityVc/include;$durabilitySdk/Include"
$env:LIB = "$durabilityVc/lib/amd64;$durabilitySdk/Lib/x64"
& "$durabilityVc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$durabilityOut/rules.obj" "/Fe$durabilityOut/rules.exe" "$PSScriptRoot/condition_rules.cpp"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$durabilityOut/rules.exe"
exit $LASTEXITCODE
