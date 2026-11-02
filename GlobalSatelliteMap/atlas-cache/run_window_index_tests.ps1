param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$out=Join-Path $PSScriptRoot '_generated/window-index-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$saved=$env:GSI_TEST_MODULE
try {
    Remove-Item Env:GSI_TEST_MODULE -ErrorAction SilentlyContinue
    & $python "$PSScriptRoot/test_window_index.py" *> "$out/fresh-suite.log"
    if($LASTEXITCODE -ne 0){throw "Index fixture suite failed; inspect $out/fresh-suite.log"}
    Get-Content "$out/fresh-suite.log" -Tail 5
    if(-not $SkipMutation){
        $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'build_window_index.py'))
        $mutants=@(
            @('skip-whole-plane-sha',"if whole.hexdigest()!=d['sha256']:",'if False:','test_whole_plane_corruption_refuses_ready_pin','ValueError not raised'),
            @('transpose-window-record','+z*n+x','+x*n+z','test_all_records_exact_and_one_pass','AssertionError'),
            @('skip-capture-pin',"hashlib.sha256(capture_raw).hexdigest()!=ready['captureMetadataSha256']",'False','test_wrong_header_capture_and_missing_tile','ValueError not raised')
        )
        foreach($m in $mutants){
            if($source.IndexOf($m[1]) -lt 0){throw "Mutation anchor missing: $($m[0])"}
            $path=Join-Path $out "$($m[0]).py"
            [IO.File]::WriteAllText($path,$source.Replace($m[1],$m[2]),[Text.UTF8Encoding]::new($false))
            $env:GSI_TEST_MODULE=$path
            & $python "$PSScriptRoot/test_window_index.py" "IndexTests.$($m[3])" *> "$out/$($m[0]).log"
            $failed=$LASTEXITCODE -ne 0
            $log=[IO.File]::ReadAllText("$out/$($m[0]).log")
            if(-not $failed -or -not $log.Contains($m[4])){throw "Mutant failed unfairly or survived: $($m[0]); inspect log"}
            Write-Output "Expected RED: $($m[0]) -> $($m[3])"
        }
    }
} finally {
    if($null -eq $saved){Remove-Item Env:GSI_TEST_MODULE -ErrorAction SilentlyContinue}else{$env:GSI_TEST_MODULE=$saved}
}
exit 0
