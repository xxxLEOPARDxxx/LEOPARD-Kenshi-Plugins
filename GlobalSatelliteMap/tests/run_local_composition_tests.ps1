param()
$ErrorActionPreference='Stop'
$base=Split-Path $PSScriptRoot -Parent
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$out=Join-Path $base 'build/local-composition-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
foreach($name in @('local_revision_tests','local_mapping_tests')){
 & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "$PSScriptRoot/$name.cpp" "/Fe$out/$name.exe" "/Fo$out/$name.obj"
 if($LASTEXITCODE -ne 0){throw "Build failed: $name"}
 & "$out/$name.exe"
 if($LASTEXITCODE -ne 0){throw "Tests failed: $name"}
}
& $python "$PSScriptRoot/local_composition_tests.py"
if($LASTEXITCODE -ne 0){throw 'Actual local caller extraction failed'}
