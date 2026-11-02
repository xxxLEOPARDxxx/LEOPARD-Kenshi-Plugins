param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated/window-reader'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/Fe$out/window_reader.exe" "/Fo$out/window_reader.obj" "$PSScriptRoot/window_reader_tests.cpp" /link advapi32.lib
if($LASTEXITCODE -ne 0){throw 'Window reader compile failed'}
& "$out/window_reader.exe" $out
if($LASTEXITCODE -ne 0){throw 'Window reader tests failed'}
if(-not $SkipMutation){
    $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'AtlasWindowReader.h'))
    $mutants=@(
        @('wrong-source-level','PlaneOffset(key.level,p)','PlaneOffset(5,p)','read actual source window'),
        @('skip-window-sha','std::memcmp(actual,expectedWindowHashes[p],32)','0','window checksum mismatch discards all planes'),
        @('skip-index-sha','std::memcmp(actual,expectedIndex,32)','0','full index SHA checked'),
        @('wrong-record','unsigned record=WindowRecord(key);','unsigned record=WindowRecord(WindowKey(key.tile,5,0,0));','record tile-level-row-column mapping'),
        @('retain-partial-planes','if(!ok)r.Clear();return ok;','return ok;','window row cancellation frees every plane')
    )
    foreach($m in $mutants){
        if($source.IndexOf($m[1]) -lt 0){throw "Mutation anchor missing: $($m[0])"}
        $header=Join-Path $out "$($m[0]).h"
        [IO.File]::WriteAllText($header,$source.Replace($m[1],$m[2]),[Text.Encoding]::ASCII)
        $define='/DWINDOW_READER_HEADER="'+$header.Replace('\','/')+'"'
        & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 $define "/Fe$out/$($m[0]).exe" "/Fo$out/$($m[0]).obj" "$PSScriptRoot/window_reader_tests.cpp" /link advapi32.lib *> "$out/$($m[0])-build.log"
        if($LASTEXITCODE -ne 0){throw "Mutant compile failed: $($m[0]); inspect build log"}
        & "$out/$($m[0]).exe" $out *> "$out/$($m[0])-run.log"
        $failed=$LASTEXITCODE -ne 0
        $log=[IO.File]::ReadAllText("$out/$($m[0])-run.log")
        if(-not $failed -or -not $log.Contains("FAIL $($m[3])")){throw "Mutant failed unfairly or survived: $($m[0]): $log"}
        Write-Output "Expected RED: $($m[0]) -> $($m[3])"
    }
}
exit 0
