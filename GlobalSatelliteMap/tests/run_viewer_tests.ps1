$ErrorActionPreference='Stop'
$base=Split-Path $PSScriptRoot -Parent
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $base 'build'
$env:INCLUDE="$vc/include;$sdk/Include;D:/DEV/Kenshi/deps/boost_1_60_0"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& $python -c "from pathlib import Path; p=Path(r'$base/GlobalSatelliteMap.cpp').read_text(); a=p.index('bool Geometry('); b=p.index('void EnsureLayer(',a); Path(r'$out/ExtractedGeometry.h').write_text(p[a:b])"
if($LASTEXITCODE -ne 0){throw 'Production geometry extraction failed'}
foreach($test in @('atlas_manifest_tests','atlas_job_tests','atlas_geometry_tests','atlas_clock_tests')){
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/$test.exe" "/Fo$out/$test.obj" "$PSScriptRoot/$test.cpp" /link advapi32.lib user32.lib
    if($LASTEXITCODE -ne 0){throw "Compile failed: $test"}
    & "$out/$test.exe" "$out/worker-fixture.bin"
    if($LASTEXITCODE -ne 0){throw "Tests failed: $test"}
}
& $python "$PSScriptRoot/native_sampler_tests.py"
if($LASTEXITCODE -ne 0){throw 'Native sampler contract failed'}
& $python "$PSScriptRoot/source_contracts.py"
if($LASTEXITCODE -ne 0){throw 'Viewer source/catalog contracts failed'}
