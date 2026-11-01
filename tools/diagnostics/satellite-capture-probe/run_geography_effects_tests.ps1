$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot 'build/geography'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& $python "$PSScriptRoot/generate_geography_effects.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
foreach($variant in @('reference','owned')) {
    foreach($stage in @('vs','ps')) {
        $arguments=@('/nologo','/T',"${stage}_4_0",'/E',"fixture_$stage",'/O3','/Gec','/D','TEXTURED=1',"/Fo$out/effects-$variant-$stage.cso","$out/effects-$variant.hlsl")
        if($variant -eq 'reference'){$arguments+=@('/D','REFERENCE=1')}
        & $fxc @arguments
        if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    }
}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/geography_effects_warp.exe" "/Fo$out/geography_effects_warp.obj" "$PSScriptRoot/geography_effects_warp.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/geography_effects_warp.exe" "$out/effects-reference-vs.cso" "$out/effects-owned-vs.cso" "$out/effects-reference-ps.cso" "$out/effects-owned-ps.cso" "$out/effects-distortion.bin"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/test_geography_effects_mutations.py"
exit $LASTEXITCODE
