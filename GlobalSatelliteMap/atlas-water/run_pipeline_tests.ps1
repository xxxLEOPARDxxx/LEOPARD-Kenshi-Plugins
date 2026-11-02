param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$out=Join-Path $PSScriptRoot '_generated/pipeline'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$common='D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common'
$post='D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post'
& $python "$PSScriptRoot/generate_pipeline.py"
if($LASTEXITCODE -ne 0){throw 'Production pipeline shader generation failed'}
& $fxc /nologo /T cs_5_0 /E main /O3 /Gec /D DX11=1 /I $common /I $post "/Fo$out/TerrainOracle.cso" "$PSScriptRoot/PipelineTerrainOracle.hlsl"
if($LASTEXITCODE -ne 0){throw 'Actual native terrain oracle compile failed'}
& $fxc /nologo /T ps_5_0 /E waterFP /O3 /Gec /D DISTANT=1 /D NO_REFLECTION=1 /I $common "/Fo$out/NativeWater.cso" 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/forward/water.hlsl'
if($LASTEXITCODE -ne 0){throw 'Actual native water compile failed'}
& $fxc /nologo /T vs_5_0 /E main /O3 /Gec "/Fo$out/ReferenceVS.cso" "$PSScriptRoot/ReferenceVS.hlsl"
if($LASTEXITCODE -ne 0){throw 'Native water reference VS failed'}
& $fxc /nologo /T ps_5_0 /E main /O3 /Gec "/Fo$out/ReferenceTerrainPS.cso" "$PSScriptRoot/ReferenceTerrainPS.hlsl"
if($LASTEXITCODE -ne 0){throw 'Native water depth fixture failed'}
& $fxc /nologo /T cs_5_0 /E main /O3 /Gec /D DX11=1 /I $common /I $post "/Fo$out/ToneOracle.cso" "$PSScriptRoot/PipelineToneOracle.hlsl"
if($LASTEXITCODE -ne 0){throw 'Native exposure oracle failed'}
& $fxc /nologo /T cs_5_0 /E main /O3 /Gec /D DX11=1 /I $common /I $post "/Fo$out/TerrainDefault.cso" "$PSScriptRoot/../atlas-view/AtlasRelight.hlsl"
if($LASTEXITCODE -ne 0){throw 'Default terrain shader failed'}
& $python "$PSScriptRoot/verify_pipeline_shaders.py"
if($LASTEXITCODE -ne 0){throw 'Actual pipeline DXBC interface failed'}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 /wd4127 "/Fo$out/policy.obj" "/Fe$out/policy.exe" "$PSScriptRoot/pipeline_policy_tests.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){throw 'Pipeline policy compile failed'}
& "$out/policy.exe"
if($LASTEXITCODE -ne 0){throw 'Pipeline policy failed'}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$out/warp.obj" "/Fe$out/warp.exe" "$PSScriptRoot/pipeline_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){throw 'Pipeline WARP compile failed'}
& "$out/warp.exe" "$out/TerrainHdr.cso" "$out/TerrainOracle.cso" "$out/Water.cso" "$out/NativeWater.cso" "$out/ReferenceVS.cso" "$out/ReferenceTerrainPS.cso" "$out/Tone.cso" "$out/ToneOracle.cso" "$out/TerrainDefault.cso"
$rc=$LASTEXITCODE
if($rc -ne 0){throw 'Pipeline WARP failed'}
if(-not $SkipMutation){
 & $python "$PSScriptRoot/test_pipeline_mutations.py"
 if($LASTEXITCODE -ne 0){throw 'Pipeline functional negative controls failed'}
}
