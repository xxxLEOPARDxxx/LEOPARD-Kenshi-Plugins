$ErrorActionPreference='Stop'
$here=$PSScriptRoot
$out=Join-Path $here '_generated'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE="$vc/include;$sdk/Include;$here"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$test=Join-Path $here 'writer_tests.cpp'
$exe=Join-Path $out 'writer_tests.exe'
$obj=Join-Path $out 'writer_tests.obj'
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /W4 /O2 $test "/Fo$obj" "/Fe$exe"
if($LASTEXITCODE -eq 0){& $exe (Join-Path $out ('win32-'+[guid]::NewGuid().ToString('N')+'.tmp'))}
if($LASTEXITCODE -ne 0){throw "writer tests failed: $LASTEXITCODE"}
