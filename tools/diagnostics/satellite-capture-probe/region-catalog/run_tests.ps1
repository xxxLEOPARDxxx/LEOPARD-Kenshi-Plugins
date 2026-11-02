$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
foreach($script in @('generate_catalog.py','test_generator.py','make_test_fixture.py')){
    & $python "$PSScriptRoot/$script"
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 /WX "/Fe$out/catalog_tests.exe" "/Fo$out/catalog_tests.obj" "$PSScriptRoot/catalog_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/catalog_tests.exe" "$out/source.fixture"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
foreach($script in @('test_adapters.py','test_parameters.py')){
    & $python "$PSScriptRoot/$script"
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges "/Fo$out/region_sampling.cso" "$PSScriptRoot/region_sampling_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 /WX "/Fe$out/region_sampling_warp.exe" "/Fo$out/region_sampling_warp.obj" "$PSScriptRoot/region_sampling_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/region_sampling_warp.exe" "$out/region_sampling.cso" "$out/blend.rgba"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
foreach($script in @('test_mutants.py','test_gpu_mutants.py','test_region_bindings.py')){
    & $python "$PSScriptRoot/$script"
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
