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
foreach($test in @('atlas_manifest_tests','atlas_job_tests','atlas_geometry_tests','atlas_clock_tests','atlas_window_policy_tests','atlas_pan_stability_tests','local_diagnostic_tests','zoom_tests','zoom_step_tests')){
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/$test.exe" "/Fo$out/$test.obj" "$PSScriptRoot/$test.cpp" /link advapi32.lib user32.lib
    if($LASTEXITCODE -ne 0){throw "Compile failed: $test"}
    & "$out/$test.exe" "$out/worker-fixture.bin"
    if($LASTEXITCODE -ne 0){throw "Tests failed: $test"}
}
& $python "$PSScriptRoot/native_sampler_tests.py"
if($LASTEXITCODE -ne 0){throw 'Native sampler contract failed'}
& $python "$PSScriptRoot/managed_image_tests.py"
if($LASTEXITCODE -ne 0){throw 'Extracted managed image completion failed'}
& $python "$PSScriptRoot/source_contracts.py"
if($LASTEXITCODE -ne 0){throw 'Viewer source/catalog contracts failed'}

foreach($test in @('window_geometry_tests.py','window_adoption_tests.py','window_mutation_tests.py','presentation_caller_tests.py')){
    & $python "$PSScriptRoot/$test"
    if($LASTEXITCODE -ne 0){throw "Window integration failed: $test"}
}
& $python "$PSScriptRoot/water_caller_tests.py"
if($LASTEXITCODE -ne 0){throw 'Extracted water caller lifecycle failed'}
& "$PSScriptRoot/run_water_job_tests.ps1"
if($LASTEXITCODE -ne 0){throw 'Water-source worker/window minimum failed'}
foreach($test in @('model_job_tests.py','model_caller_tests.py','cached_town_job_tests.py','cached_town_job_mutations.py','cached_town_state_tests.py','cached_town_caller_tests.py','zoom_mutation_tests.py','zoom_caller_tests.py','zoom_step_caller_tests.py')){
    & $python "$PSScriptRoot/$test"
    if($LASTEXITCODE -ne 0){throw "Model integration failed: $test"}
}
exit 0
