$ErrorActionPreference='Stop'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/shader-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges "/Fo$out/painted.cso" "$PSScriptRoot/painted_map_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T cs_5_0 /E main /O3 /Gec /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post' "/Fo$out/exposure.cso" "$PSScriptRoot/native_exposure_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges "/Fo$out/position.cso" "$PSScriptRoot/orthographic_position_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T cs_5_0 /E reference_main /O3 /Ges "/Fo$out/reference.cso" "$PSScriptRoot/orthographic_position_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T cs_5_0 /E main /O3 /Ges "/Fo$out/decoder.cso" "$PSScriptRoot/gbuffer_decode_cs.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E main /O3 /Ges "/Fo$out/position_pixel.cso" "$PSScriptRoot/orthographic_position_ps.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T vs_4_0 /E main_vs /O3 /Ges "/Fo$out/probe_vertex.cso" "$PSScriptRoot/../shaders/PositionProbe.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E main_ps /O3 /Ges "/Fo$out/probe_pixel.cso" "$PSScriptRoot/../shaders/PositionProbe.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $fxc /nologo /T ps_4_0 /E main_ps /O3 /Gec /D DX11=1 /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/deferred' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post' "/Fo$out/sun_pixel.cso" "$PSScriptRoot/../shaders/SunProbe.hlsl"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_shader_bindings.py" "$out/sun_pixel.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/position_warp.exe" "/Fo$out/position_warp.obj" "$PSScriptRoot/orthographic_position_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/position_warp.exe" "$out/position.cso"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/position_warp.exe" "$out/reference.cso" reference
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/position_warp.exe" "$out/decoder.cso" decode
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/position_warp.exe" "$out/exposure.cso" exposure
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/position_warp.exe" "$out/painted.cso" painted
exit $LASTEXITCODE
