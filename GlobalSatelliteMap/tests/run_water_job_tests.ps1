param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$base=Split-Path $PSScriptRoot -Parent
$out=Join-Path $base 'build/water-job-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$extract=@'
from pathlib import Path
import sys
p=Path(sys.argv[1]).read_text();a=p.index('inline DWORD WINAPI Run(');b=p.index('{',a);d=1;e=b+1
while d:
 if p[e]=='{':d+=1
 elif p[e]=='}':d-=1
 e+=1
# Bind only the dependency seams; keep production branch/order/arguments.
body=p[a:e]
for name in ('Validate','ValidateWaterSources','WindowMinimum'):
 body=body.replace(name+'(job)','Dispatch::'+name+'(job)')
Path(sys.argv[2]).write_text(body)
'@
& $python -c $extract (Join-Path $base 'GsmAtlasJob.h') (Join-Path $out 'ExtractedWaterJobRun.h')
if($LASTEXITCODE -ne 0){throw 'Production worker dispatch extraction failed'}
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE="$vc/include;$sdk/Include;D:/DEV/Kenshi/deps/boost_1_60_0;$out;$base"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out/tests.obj" "/Fe$out/tests.exe" "$PSScriptRoot/atlas_water_job_tests.cpp" /link advapi32.lib user32.lib
if($LASTEXITCODE -ne 0){throw 'Water worker tests compile failed'}
& "$out/tests.exe" ($out.Replace('\','/'))
if($LASTEXITCODE -ne 0){throw 'Water worker tests failed'}
if(-not $SkipMutation){
    $source=[IO.File]::ReadAllText((Join-Path $base 'GsmAtlasJob.h'))
    $mutants=@(
        @('include-empty-coverage','if(!job.read.plane[1][i*4+3])continue;','/* omit validity */','all zero coverage has no height'),
        @('include-nonfinite-height','_finite(height)&&(!found||height<minimum)','(!found||height<minimum)','all covered nonfinite samples do not invent a height'),
        @('skip-water-mode','if(job.waterValidation){ValidateWaterSources(job);','if(false){ValidateWaterSources(job);','actual water-only worker hashes all generated source pins'),
        @('expand-source-budget','16ULL*1024*1024','64ULL*1024*1024','sixteen MiB source cap refuses before reading oversized file')
    )
    foreach($m in $mutants){
        if(-not $source.Contains($m[1])){throw "Water worker mutant anchor absent: $($m[0])"}
        $header=Join-Path $out "$($m[0]).h"
        [IO.File]::WriteAllText($header,$source.Replace($m[1],$m[2]),[Text.Encoding]::ASCII)
        $define='/DWATER_JOB_HEADER="'+$header.Replace('\','/')+'"'
        & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 $define "/Fo$out/$($m[0]).obj" "/Fe$out/$($m[0]).exe" "$PSScriptRoot/atlas_water_job_tests.cpp" /link advapi32.lib user32.lib *> "$out/$($m[0])-build.log"
        if($LASTEXITCODE -ne 0){throw "Water worker mutant compile failed: $($m[0])"}
        & "$out/$($m[0]).exe" ($out.Replace('\','/')) *> "$out/$($m[0])-run.log"
        $failed=$LASTEXITCODE -ne 0
        $log=[IO.File]::ReadAllText("$out/$($m[0])-run.log")
        if(-not $failed -or -not $log.Contains("FAIL $($m[3])")){throw "Water worker mutant survived or failed unfairly: $($m[0])"}
        Write-Output "Expected RED: $($m[0])"
    }
}
exit 0
