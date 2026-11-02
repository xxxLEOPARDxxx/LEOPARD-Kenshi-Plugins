param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated/native-snapshot'
New-Item -ItemType Directory -Force $out | Out-Null
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python "$PSScriptRoot/extract_native_snapshot_dispatch.py"
if($LASTEXITCODE -ne 0){throw 'Production dispatch extraction failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot;$out"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$out/tests.obj" "/Fe$out/tests.exe" "$PSScriptRoot/native_snapshot_tests.cpp" /link d3d11.lib
if($LASTEXITCODE -ne 0){throw 'Native snapshot policy compile failed'}
& "$out/tests.exe"
if($LASTEXITCODE -ne 0){throw 'Native snapshot policy tests failed'}
$kl='D:/DEV/Kenshi/KenshiLib-src/Include'
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot;$PSScriptRoot/.."
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "/Fo$out/adapter.obj" "$PSScriptRoot/native_snapshot_compile.cpp" *> "$out/adapter-compile.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/adapter-compile.log" -Tail 20;throw 'Native snapshot adapter compile failed'}
Write-Output 'Native snapshot complete adapter VC10 compile PASS (not linked or invoked)'
& $python "$PSScriptRoot/audit_native_snapshot.py"
if($LASTEXITCODE -ne 0){throw 'Native snapshot source/profile audit failed'}
if(-not $SkipMutation){
    $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'NativeWaterSnapshot.h'))
    $env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot;$out"
    $mutants=@(
        @('water-sun-w-only','u.waterSunColour[0]=u.waterSunColour[1]=u.waterSunColour[2]=0','u.waterSunColour[3]=0','sun OFF zeros water RGB while retaining native W'),
        @('omit-native-mip-bias','out.MipLODBias=bias;','out.MipLODBias=0;','sampler bias and FO_NONE unlimited MaxLOD preserved'),
        @('reject-native-one-mip','AtlasView::LightView(views[i],d.p,i>=3,1)','AtlasView::LightView(views[i],d.p,i>=3,i==4?8:1)','actual one-mip water specular cube admitted'),
        @('omit-typed-definition','type==count&&available','true&&available','integer or wrong constant type rejected'),
        @('double-retain-native-srv','views[i]->AddRef();views_[i].Reset();','views[i]->AddRef();views[i]->AddRef();views_[i].Reset();','snapshot independently retains native SRV COM reference'),
        @('wrong-native-water-root','WaterRootIndex=1;','WaterRootIndex=0;','actual native water root index1 selected instead of SDK dynamic0'),
        @('active-unit-getter','const void*slot=NULL;if(!PassiveFrame(u,slot))throw std::runtime_error("native water existing texture frame unavailable");','typedef const Ogre::TexturePtr&(__fastcall*Get)(const Ogre::TextureUnitState*);out=reinterpret_cast<Get>(Pinned(0x37dbd0))(u);return;const void*slot=NULL;','unloaded existing texture refused without native loading')
    )
    try {foreach($m in $mutants){
        if(-not $source.Contains($m[1])){throw "Mutant anchor missing: $($m[0])"}
        $header=Join-Path $out "$($m[0]).h"
        [IO.File]::WriteAllText($header,$source.Replace($m[1],$m[2]),[Text.Encoding]::ASCII)
        $define='/DNATIVE_WATER_HEADER="'+$header.Replace('\','/')+'"'
        & $python "$PSScriptRoot/extract_native_snapshot_dispatch.py" --source $header
        if($LASTEXITCODE -ne 0){throw "Mutated production extraction failed: $($m[0])"}
        $env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot;$PSScriptRoot/..;$out"
        & "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE $define "/Fo$out/$($m[0])-adapter.obj" "$PSScriptRoot/native_snapshot_compile.cpp" *> "$out/$($m[0])-adapter-build.log"
        if($LASTEXITCODE -ne 0){throw "Full native mutant adapter compile failed: $($m[0])"}
        & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 $define "/Fo$out/$($m[0]).obj" "/Fe$out/$($m[0]).exe" "$PSScriptRoot/native_snapshot_tests.cpp" /link d3d11.lib *> "$out/$($m[0])-build.log"
        if($LASTEXITCODE -ne 0){throw "Native snapshot mutant compile failed: $($m[0])"}
        & "$out/$($m[0]).exe" *> "$out/$($m[0])-run.log"
        $failed=$LASTEXITCODE -ne 0
        $log=[IO.File]::ReadAllText("$out/$($m[0])-run.log")
        if(-not $failed -or -not $log.Contains("FAIL $($m[3])")){throw "Mutant survived or failed unfairly: $($m[0]); inspect log"}
        Write-Output "Expected RED: $($m[0]) -> $($m[3])"
    }} finally {& $python "$PSScriptRoot/extract_native_snapshot_dispatch.py"}
}
