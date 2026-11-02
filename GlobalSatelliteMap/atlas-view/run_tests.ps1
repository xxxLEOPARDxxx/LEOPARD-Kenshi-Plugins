param([switch]$PolicyOnly,[switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 /wd4127 "/Fe$out/policy.exe" "/Fo$out/policy.obj" "$PSScriptRoot/policy_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Policy compilation failed'}
& "$out/policy.exe"
if($LASTEXITCODE -ne 0){throw 'Policy tests failed'}
if($PolicyOnly){exit 0}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/reader.exe" "/Fo$out/reader.obj" "$PSScriptRoot/reader_tests.cpp" /link advapi32.lib
if($LASTEXITCODE -ne 0){throw 'Reader compilation failed'}
& "$out/reader.exe" "$out/fixture.gst"
if($LASTEXITCODE -ne 0){throw 'Reader tests failed'}
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
foreach($shader in @('AtlasRelight','reference_cs')){
    & $fxc /nologo /T cs_5_0 /E main /O3 /Gec /D DX11=1 /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post' "/Fo$out/$shader.cso" "$PSScriptRoot/$shader.hlsl"
    if($LASTEXITCODE -ne 0){throw "Shader compilation failed: $shader"}
}
foreach($stage in @('vs','ps')){
    & $fxc /nologo /T "${stage}_5_0" /E $stage /O3 /Ges "/Fo$out/$stage.cso" "$PSScriptRoot/../tests/gpu_mip_state.hlsl"
    if($LASTEXITCODE -ne 0){throw 'State shader compilation failed'}
}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/warp.exe" "/Fo$out/warp.obj" "$PSScriptRoot/warp_tests.cpp" /link d3d11.lib advapi32.lib
if($LASTEXITCODE -ne 0){throw 'WARP compilation failed'}
& "$out/warp.exe" "$out/AtlasRelight.cso" "$out/reference_cs.cso" "$out/vs.cso" "$out/ps.cso"
if($LASTEXITCODE -ne 0){throw 'WARP tests failed'}
if(!$SkipMutation){
    function Run-Mutant([string]$name,[string]$exe,[string[]]$arguments,[string]$expected){
        $proc=Start-Process -FilePath $exe -ArgumentList $arguments -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput "$out/$name.stdout.log" -RedirectStandardError "$out/$name.stderr.log"
        if($proc.ExitCode -eq 0){throw "Mutant incorrectly passed: $name"}
        if(!(Select-String -LiteralPath "$out/$name.stderr.log" -SimpleMatch $expected -Quiet)){Get-Content "$out/$name.stderr.log";throw "Mutant failed for wrong reason: $name"}
        Write-Output "PASS mutant rejected: $name ($expected)"
    }
    $source=[IO.File]::ReadAllText("$PSScriptRoot/AtlasRelight.h")
    $needle='c_->CSSetShader(shader_,classes_,count_);'
    if(([regex]::Matches($source,[regex]::Escape($needle))).Count -ne 1){throw 'State mutation site missing'}
    $header="$out/AtlasRelight.omit-state.h"
    [IO.File]::WriteAllText($header,$source.Replace($needle,'/* mutant omits CS restore */'),[Text.Encoding]::ASCII)
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DATLAS_RELIGHT_HEADER=`"$($header.Replace('\','/'))`"" "/Fe$out/state-mutant.exe" "/Fo$out/state-mutant.obj" "$PSScriptRoot/warp_tests.cpp" /link d3d11.lib advapi32.lib
    if($LASTEXITCODE -ne 0){throw 'State mutant build failed'}
    Run-Mutant 'omit-state' "$out/state-mutant.exe" @("$out/AtlasRelight.cso","$out/reference_cs.cso","$out/vs.cso","$out/ps.cso") 'complete D3D11.0 pipeline snapshot identical'
    $needle='Set(stage,slot,views_[stage][slot]);views_[stage][slot]->Release();'
    if(([regex]::Matches($source,[regex]::Escape($needle))).Count -ne 1){throw 'Owned output mutation site missing'}
    $header="$out/AtlasRelight.omit-owned-read-restore.h"
    [IO.File]::WriteAllText($header,$source.Replace($needle,'/* mutant omits owned read restore */ views_[stage][slot]->Release();'),[Text.Encoding]::ASCII)
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DATLAS_RELIGHT_HEADER=`"$($header.Replace('\','/'))`"" "/Fe$out/owned-read-mutant.exe" "/Fo$out/owned-read-mutant.obj" "$PSScriptRoot/warp_tests.cpp" /link d3d11.lib advapi32.lib
    if($LASTEXITCODE -ne 0){throw 'Owned output mutant build failed'}
    Run-Mutant 'omit-owned-read-restore' "$out/owned-read-mutant.exe" @("$out/AtlasRelight.cso","$out/reference_cs.cso","$out/vs.cso","$out/ps.cso") 'owned output SRV all-stage exact restoration'
    $source=[IO.File]::ReadAllText("$PSScriptRoot/AtlasReader.h")
    $needle='std::memcmp(actual,header+128+(r.level*3+p)*48+16,32)'
    if(([regex]::Matches($source,[regex]::Escape($needle))).Count -ne 1){throw 'Hash mutation site missing'}
    $header="$out/AtlasReader.omit-hash.h"
    [IO.File]::WriteAllText($header,$source.Replace($needle,'0'),[Text.Encoding]::ASCII)
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DATLAS_READER_HEADER=`"$($header.Replace('\','/'))`"" "/Fe$out/hash-mutant.exe" "/Fo$out/hash-mutant.obj" "$PSScriptRoot/reader_tests.cpp" /link advapi32.lib
    if($LASTEXITCODE -ne 0){throw 'Hash mutant build failed'}
    Run-Mutant 'omit-hash' "$out/hash-mutant.exe" @("$out/hash-fixture.gst") 'payload corruption rejected all planes'
    $source=[IO.File]::ReadAllText("$PSScriptRoot/AtlasRelight.hlsl")
    foreach($mutation in @(
        @{Name='wrong-block-origin';Needle='uint2 global=id.xy+uint2(dispatchWindow.xy);';Replacement='uint2 global=id.xy;';Expected='global block origins and partial coverage'},
        @{Name='omit-coverage';Needle='float4(colour,encoded.a)';Replacement='float4(colour,1)';Expected='fractional coverage preserved'},
        @{Name='clip-hdr';Needle='float3 normal=normalize(n), albedo=material.rgb;';Replacement='float3 normal=normalize(n), albedo=saturate(material.rgb);';Expected='reflectance1.125 must exceed1 before exposure without clipping'}
    )){
        if(([regex]::Matches($source,[regex]::Escape($mutation.Needle))).Count -ne 1){throw "Shader mutation site missing: $($mutation.Name)"}
        $mutated=$source.Replace($mutation.Needle,$mutation.Replacement).Replace('../shaders/NativeExposure.hlsl','../../shaders/NativeExposure.hlsl')
        [IO.File]::WriteAllText("$out/$($mutation.Name).hlsl",$mutated,[Text.Encoding]::ASCII)
        & $fxc /nologo /T cs_5_0 /E main /O3 /Gec /D DX11=1 /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common' /I 'D:/SteamLibrary/steamapps/common/Kenshi/data/materials/post' "/Fo$out/$($mutation.Name).cso" "$out/$($mutation.Name).hlsl"
        if($LASTEXITCODE -ne 0){throw 'Shader mutant build failed'}
        Run-Mutant $mutation.Name "$out/warp.exe" @("$out/$($mutation.Name).cso","$out/reference_cs.cso","$out/vs.cso","$out/ps.cso") $mutation.Expected
    }
}
