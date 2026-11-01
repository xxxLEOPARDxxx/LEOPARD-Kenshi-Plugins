$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot 'build/geography'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/geography_completion_tests.exe" "/Fo$out/geography_completion_tests.obj" "$PSScriptRoot/geography_completion_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_completion_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python "$PSScriptRoot/generate_geography_inputs.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/geography_upload_tests.exe" "/Fo$out/geography_upload_tests.obj" "$PSScriptRoot/geography_upload_tests.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_upload_tests.exe" 'D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/textures/GrassBadTestDIF.dds'
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/generate_geography_fixture.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges /I $PSScriptRoot "/Fo$out/geography_sampling.cso" "$PSScriptRoot/geography_sampling_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/I$PSScriptRoot/build" "/Fe$out/geography_sampling_warp.exe" "/Fo$out/geography_sampling_warp.obj" "$PSScriptRoot/geography_sampling_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_sampling_warp.exe" "$out/geography_sampling.cso" "$out/sampling.points" "$out/sampling.expected" "$out/blend.rgba"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges /I $PSScriptRoot "/Fo$out/geography_bank.cso" "$PSScriptRoot/geography_bank_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/I$PSScriptRoot/build" "/Fe$out/geography_bank_warp.exe" "/Fo$out/geography_bank_warp.obj" "$PSScriptRoot/geography_bank_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_bank_warp.exe" "$out/geography_bank.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_geography_mutations.py"
exit $LASTEXITCODE
