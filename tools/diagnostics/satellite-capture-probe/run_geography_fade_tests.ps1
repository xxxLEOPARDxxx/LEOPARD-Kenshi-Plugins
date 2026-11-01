$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot 'build/geography'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot;$PSScriptRoot/build;$out"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
foreach($script in @('embed_painted_shader.py','embed_geography_shader.py','generate_geography_fade.py')) {
    & $python "$PSScriptRoot/$script"
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/geography_fade_tests.exe" "/Fo$out/geography_fade_tests.obj" "$PSScriptRoot/geography_fade_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_fade_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_geography_fade_route.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T vs_4_0 /E fixture_vs /O3 /Gec /D REFERENCE=1 "/Fo$out/fade-vs.cso" "$out/fade-reference.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E fixture_ps /O3 /Gec /D REFERENCE=1 "/Fo$out/fade-reference-ps.cso" "$out/fade-reference.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E fixture_ps /O3 /Gec "/Fo$out/fade-owned-ps.cso" "$out/fade-owned.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/geography_fade_warp.exe" "/Fo$out/geography_fade_warp.obj" "$PSScriptRoot/geography_fade_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_fade_warp.exe" "$out/fade-vs.cso" "$out/fade-reference-ps.cso" "$out/fade-owned-ps.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_geography_fade_mutations.py"
exit $LASTEXITCODE
