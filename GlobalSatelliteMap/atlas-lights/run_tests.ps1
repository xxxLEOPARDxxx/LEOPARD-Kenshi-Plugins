param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
& $python "$PSScriptRoot/audit_profile.py"
if($LASTEXITCODE -ne 0){throw 'Local-light native audit failed'}
& $python "$PSScriptRoot/extract_backend.py"
if($LASTEXITCODE -ne 0){throw 'Actual native backend extraction failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot;$out"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$out/tests.obj" "/Fe$out/tests.exe" "$PSScriptRoot/snapshot_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Local-light tests compile failed'}
& "$out/tests.exe" "$out/clock-fixture.bin"
if($LASTEXITCODE -ne 0){throw 'Local-light tests failed'}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fo$out/backend.obj" "/Fe$out/backend.exe" "$PSScriptRoot/backend_tests.cpp"
if($LASTEXITCODE -ne 0){throw 'Extracted backend compile failed'}
& "$out/backend.exe" "$out/origin-fixture.bin"
if($LASTEXITCODE -ne 0){throw 'Extracted backend tests failed'}
$kl='D:/DEV/Kenshi/KenshiLib-src/Include'
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot;$PSScriptRoot/..;$out"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "/Fo$out/adapter.obj" "$PSScriptRoot/compile_adapter.cpp" *> "$out/adapter-compile.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/adapter-compile.log" -Tail 10;throw 'Native local-light adapter compile failed'}
Write-Output 'Complete native local-light adapter VC10 compile PASS (not linked/invoked)'
if(-not $SkipMutation){
    $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'NativeLocalLightSnapshot.h'))
    $mutants=@(
        @('limit32','Limit=4096;','Limit=32;','core','all4096 copied lights retained without32 total limit'),
        @('silent-overflow-clamp','if(state.count>Limit)return Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW),"registered native light count exceeds4096");','if(state.count>Limit)state.count=Limit;','core','4097 overflow refuses before enumeration and clears output'),
        @('off-native-query','if(!enabled){Reset();return S_FALSE;}','if(!enabled){native.Profile(generation);Reset();return S_FALSE;}','core','OFF makes zero native queries'),
        @('hardcode-clock','v.time=float(c.time*60.0);','v.time=0;','core','current native clock values copied'),
        @('rounded-ramp-denominator','(c.phase-c.start)/(up-c.start)','(c.phase-c.start)/e','core','exact extracted native clock float ramp'),
        @('omit-effect3','v.effect3=float(double(product)*.05+.95);','v.effect3=1;','core','native sinf effect operation order'),
        @('ignore-daytime-gate','if(value.custom0[0]>0&&info.clock.daytime>.1f)','if(false)','core','actual visibility power and daytime suppression'),
        @('wrong-position-owner','reinterpret_cast<Vector>(functions_[FPosition])(parent,out.worldPosition)','reinterpret_cast<Vector>(functions_[FPosition])(p,out.worldPosition)','backend','actual native point-light getter dispatch and sret'),
        @('full-spot-angle','const float halfInner=inner*.5f,halfOuter=outer*.5f;','const float halfInner=inner,halfOuter=outer;','backend','actual native spot half-angle cosine dispatch'),
        @('uncached-profile','if(attempted_&&epoch_==generation)','if(false)','backend','actual profile cached once per generation'),
        @('omit-scene-owner','if(reinterpret_cast<Pointer>(functions_[FManager])(p)!=scene)','if(false)','backend','actual native scene mismatch refuses before visibility'),
        @('retain-failed-output','HRESULT Fail(HRESULT hr,const char*s){Clear();','HRESULT Fail(HRESULT hr,const char*s){','core','4097 overflow refuses before enumeration and clears output'),
        @('omit-origin-publication','std::memcpy(info.SourceSceneOrigin,state.SourceSceneOrigin,12);','','core','coherent native origin published without transforming raw position'),
        @('ignore-origin-rebase','&&a.SourceSceneOrigin[0]==b.SourceSceneOrigin[0]&&a.SourceSceneOrigin[1]==b.SourceSceneOrigin[1]&&a.SourceSceneOrigin[2]==b.SourceSceneOrigin[2]','','backend','actual native rebase during acquisition retires old origin and payload'),
        @('ignore-origin-sret','reinterpret_cast<Vector>(functions_[FRelativeOrigin])(expectedScene,state.SourceSceneOrigin)!=state.SourceSceneOrigin','(reinterpret_cast<Vector>(functions_[FRelativeOrigin])(expectedScene,state.SourceSceneOrigin),false)','backend','native origin aggregate output identity required'),
        @('omit-origin-finite-guards','!_finite(state.SourceSceneOrigin[0])||!_finite(state.SourceSceneOrigin[1])||!_finite(state.SourceSceneOrigin[2])','false','backend','nonfinite native scene origin refused')
    )
    try {foreach($m in $mutants){
        if(-not $source.Contains($m[1])){throw "Mutation anchor missing: $($m[0])"}
        $header=Join-Path $out "$($m[0]).h"
        [IO.File]::WriteAllText($header,$source.Replace($m[1],$m[2]),[Text.Encoding]::ASCII)
        $define='/DLOCAL_LIGHT_HEADER="'+$header.Replace('\','/')+'"'
        & $python "$PSScriptRoot/extract_backend.py" --source $header
        if($LASTEXITCODE -ne 0){throw "Mutant extraction failed: $($m[0])"}
        & "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE $define "/Fo$out/$($m[0])-adapter.obj" "$PSScriptRoot/compile_adapter.cpp" *> "$out/$($m[0])-adapter-build.log"
        if($LASTEXITCODE -ne 0){throw "Mutant complete adapter compile failed: $($m[0])"}
        $testSource=if($m[3] -eq 'core'){'snapshot_tests.cpp'}else{'backend_tests.cpp'}
        & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 $define "/Fo$out/$($m[0]).obj" "/Fe$out/$($m[0]).exe" "$PSScriptRoot/$testSource" *> "$out/$($m[0])-test-build.log"
        if($LASTEXITCODE -ne 0){throw "Mutant fixture compile failed: $($m[0])"}
        if($m[3] -eq 'core'){& "$out/$($m[0]).exe" "$out/clock-fixture.bin" *> "$out/$($m[0])-run.log"}else{& "$out/$($m[0]).exe" "$out/origin-fixture.bin" *> "$out/$($m[0])-run.log"}
        $failed=$LASTEXITCODE -ne 0;$log=[IO.File]::ReadAllText("$out/$($m[0])-run.log")
        if(-not $failed -or -not $log.Contains("FAIL $($m[4])")){throw "Mutant survived or failed unfairly: $($m[0]); inspect log"}
        Write-Output "Expected RED: $($m[0]) -> $($m[4])"
    }} finally {& $python "$PSScriptRoot/extract_backend.py"}
}
