$ErrorActionPreference = "Stop"
$testPlugin = Split-Path $PSScriptRoot -Parent
$testStand = Split-Path (Split-Path $testPlugin -Parent) -Parent
$testOutput = Join-Path $testStand "build\qol-tests"
$vcRoot = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC"
$sdkRoot = "C:\Program Files\Microsoft SDKs\Windows\v7.1"
New-Item -ItemType Directory -Force $testOutput | Out-Null
$env:INCLUDE = "$vcRoot\include;$sdkRoot\Include"
$env:LIB = "$vcRoot\lib\amd64;$sdkRoot\Lib\x64"
& "$vcRoot\bin\amd64\cl.exe" /nologo /EHsc /MD /DUNICODE /D_UNICODE "/I$PSScriptRoot\mcm-fakes" "/I$testStand\ours\shared" "/I$testPlugin" "/Fo$testOutput\" "/Fe$testOutput\MiningSettingsTests.exe" "$PSScriptRoot\MiningSettingsTests.cpp" "$testPlugin\MiningSettings.cpp" /link user32.lib kernel32.lib
if ($LASTEXITCODE -ne 0) { throw "MCM editor test compilation failed" }
& "$testOutput\MiningSettingsTests.exe"
if ($LASTEXITCODE -ne 0) { throw "MCM editor tests failed" }
