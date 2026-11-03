$ErrorActionPreference='Stop'
$vc='C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC'
$sdk='C:\Program Files\Microsoft SDKs\Windows\v7.1'
$env:INCLUDE="$vc\include;$sdk\Include;D:\DEV\Kenshi\ours\shared"
$env:LIB="$vc\lib\amd64;$sdk\Lib\x64"
$env:PATH="$vc\bin\amd64;$env:PATH"
$out=Join-Path $PSScriptRoot '_build-extensions'
New-Item -ItemType Directory -Force $out | Out-Null
& "$vc\bin\amd64\cl.exe" /nologo /c /TC /W3 "/Fo$out\c-abi.obj" (Join-Path $PSScriptRoot 'extensions_c_abi.c')
if($LASTEXITCODE -ne 0) { throw 'Public C ABI/pack restoration failed' }
& "$vc\bin\amd64\cl.exe" /nologo /c /EHsc /MD /O2 /W3 "/Fo$out\sdk-example.obj" (Join-Path $PSScriptRoot '..\sdk\GradeProviderExample.cpp')
if($LASTEXITCODE -ne 0) { throw 'Public SDK example failed to compile' }
foreach($opt in @('Od','O2')) {
    & "$vc\bin\amd64\cl.exe" /nologo /EHsc /MD /W3 "/$opt" "/Fo$out\$opt.obj" "/Fe$out\$opt.exe" (Join-Path $PSScriptRoot 'extensions_tests.cpp')
    if($LASTEXITCODE -ne 0) { throw "Compile $opt failed" }
    & "$out\$opt.exe"
    if($LASTEXITCODE -ne 0) { throw "Tests $opt failed" }
    & "$vc\bin\amd64\cl.exe" /nologo /EHsc /MD /LD "/$opt" "/Fo$out\provider-$opt.obj" "/Fe$out\extensions_provider.dll" (Join-Path $PSScriptRoot 'extensions_provider.cpp')
    if($LASTEXITCODE -ne 0) { throw "Provider build $opt failed" }
    & "$vc\bin\amd64\cl.exe" /nologo /EHsc /MD /W3 "/$opt" "/Fo$out\runtime-$opt.obj" "/Fe$out\runtime-$opt.exe" (Join-Path $PSScriptRoot 'extensions_runtime.cpp')
    if($LASTEXITCODE -ne 0) { throw "Runtime build $opt failed" }
    & "$out\runtime-$opt.exe" "$out\extensions_provider.dll" "$out\profiles"
    if($LASTEXITCODE -ne 0) { throw "Runtime tests $opt failed" }
}
