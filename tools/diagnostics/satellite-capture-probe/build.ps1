$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$repo=Split-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) -Parent
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$kl='D:/DEV/Kenshi/KenshiLib-src/Include'
$boost='D:/DEV/Kenshi/deps/boost_1_60_0'
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $out | Out-Null
& $python "$PSScriptRoot/verify_render_abi.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$repo/GlobalSatelliteMap/tests/run_gpu_mip_tests.ps1"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$PSScriptRoot/run_painted_mip_tests.ps1"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$repo/GlobalSatelliteMap/tests/run_shader_tests.ps1"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/embed_position_shader.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/embed_painted_shader.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
& $fxc /nologo /T ps_4_0 /E main_fs /O3 /Gec /D DX11=1 "/Fo$out/PaintedTerrain.cso" "$out/PaintedTerrain.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_painted_shader.py" "$out/PaintedTerrain.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E main_ps /O3 /Gec /D DX11=1 /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/deferred' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post' "/Fo$out/SunShaderEmbedded.cso" "$out/SunShaderOverride.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$repo/GlobalSatelliteMap/tests/test_shader_bindings.py" "$out/SunShaderEmbedded.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;$boost;D:/DEV/Kenshi/ours/shared;$PSScriptRoot;$out"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;$boost/stage/lib"
$env:PATH="$vc/bin/amd64;$env:PATH"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GS- /GL /W3 /D NDEBUG /D WIN32 /D _WINDOWS /D _USRDLL /D UNICODE /D _UNICODE "/Fo$out/CaptureProbe.obj" "$PSScriptRoot/CaptureProbe.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/link.exe" /nologo /DLL /MACHINE:X64 /LTCG "/OUT:$out/SatelliteCaptureProbe.dll" KenshiLib.lib OgreMain_x64.lib MyGUIEngine_x64.lib user32.lib kernel32.lib advapi32.lib "$out/CaptureProbe.obj"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python (Join-Path $repo 'GlobalSatelliteMap/tests/test_pe_contract.py') "$out/SatelliteCaptureProbe.dll" --imports-root 'D:/SteamLibrary/steamapps/common/Kenshi' --report "$PSScriptRoot/loader-contract.json"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/policy_tests.exe" "/Fo$out/policy_tests.obj" "$PSScriptRoot/policy_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/policy_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/attachment_tests.exe" "/Fo$out/attachment_tests.obj" "$PSScriptRoot/attachment_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/attachment_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/pose_tests.exe" "/Fo$out/pose_tests.obj" "$PSScriptRoot/pose_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/pose_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/native_abi_tests.exe" "/Fo$out/native_abi_tests.obj" "$PSScriptRoot/native_abi_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/native_abi_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/terrain_experiment_tests.exe" "/Fo$out/terrain_experiment_tests.obj" "$PSScriptRoot/terrain_experiment_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/terrain_experiment_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/worker_handle_tests.exe" "/Fo$out/worker_handle_tests.obj" "$PSScriptRoot/worker_handle_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/worker_handle_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/exposure_tests.exe" "/Fo$out/exposure_tests.obj" "$PSScriptRoot/exposure_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/exposure_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/raster_tests.exe" "/Fo$out/raster_tests.obj" "$PSScriptRoot/raster_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/raster_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/rtv_debt_tests.exe" "/Fo$out/rtv_debt_tests.obj" "$PSScriptRoot/rtv_debt_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/rtv_debt_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/biome_selection_tests.exe" "/Fo$out/biome_selection_tests.obj" "$repo/GlobalSatelliteMap/tests/biome_selection_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/biome_selection_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/texture_array_tests.exe" "/Fo$out/texture_array_tests.obj" "$repo/GlobalSatelliteMap/tests/texture_array_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/texture_array_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/painted_map_tests.exe" "/Fo$out/painted_map_tests.obj" "$repo/GlobalSatelliteMap/tests/painted_map_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/painted_map_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_biome_reference_inputs.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_painted_inputs.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/source_contracts.py"
exit $LASTEXITCODE
