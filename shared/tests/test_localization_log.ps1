$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out='D:/DEV/Kenshi/build/LocalizationLogTests'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$env:PATH="$vc/bin/amd64;$env:PATH"
Push-Location $out
try {
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /GL /I "$PSScriptRoot" /I "$PSScriptRoot/.." "$PSScriptRoot/localization_log_tests.cpp" "$PSScriptRoot/localization_log_other.cpp" "/Fe$out/localization_log_tests.exe" /link /LTCG
    if($LASTEXITCODE -ne 0){throw 'compile failed'}
    & "$out/localization_log_tests.exe"
    if($LASTEXITCODE -ne 0){throw 'localization log tests failed'}
} finally { Pop-Location }
