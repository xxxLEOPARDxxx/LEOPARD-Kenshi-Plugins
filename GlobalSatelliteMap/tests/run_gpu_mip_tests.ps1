param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
$out=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/gpu-mip-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
foreach($stage in @('vs','ps','cs')) {
    & $fxc /nologo /T "${stage}_5_0" /E $stage /O3 /Ges "/Fo$out/$stage.cso" "$PSScriptRoot/gpu_mip_state.hlsl"
    if($LASTEXITCODE -ne 0){throw "Shader compilation failed: $stage"}
}
function Build-Test([string]$header,[string]$name) {
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DGPU_MIP_HEADER=`"$header`"" "/Fe$out/$name.exe" "/Fo$out/$name.obj" "$PSScriptRoot/gpu_mip_warp.cpp" /link d3d11.lib
    if($LASTEXITCODE -ne 0){throw "C++ compilation failed: $name"}
}
Build-Test '../GpuMipUpload.h' 'gpu_mip_warp'
& "$out/gpu_mip_warp.exe" "$out/vs.cso" "$out/ps.cso" "$out/cs.cso" 2>&1 | Tee-Object -FilePath "$out/production.log"
if($LASTEXITCODE -ne 0){throw 'GPU mip production tests failed'}
if(!$SkipMutation) {
    $header=Join-Path (Split-Path $PSScriptRoot -Parent) 'GpuMipUpload.h'
    $source=[IO.File]::ReadAllText($header)
    $needle='context.p->GenerateMips(borrowedView);'
    if(([regex]::Matches($source,[regex]::Escape($needle))).Count -ne 1){throw 'GenerateMips mutation site missing/ambiguous'}
    $mutant=Join-Path $out 'GpuMipUpload.omit-generation.h'
    [IO.File]::WriteAllText($mutant,$source.Replace($needle,'/* test-only mutant: omitted GenerateMips */'),[Text.Encoding]::ASCII)
    Build-Test $mutant.Replace('\','/') 'gpu_mip_omit_generation'
    # Native stderr is expected for this executable; capture outside the PS pipeline
    # so Windows PowerShell does not promote the expected failure into a script error.
    $proc=Start-Process -FilePath "$out/gpu_mip_omit_generation.exe" -ArgumentList @("$out/vs.cso","$out/ps.cso","$out/cs.cso") -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput "$out/mutant.stdout.log" -RedirectStandardError "$out/mutant.stderr.log"
    Get-Content -LiteralPath "$out/mutant.stdout.log"
    Get-Content -LiteralPath "$out/mutant.stderr.log"
    if($proc.ExitCode -eq 0){throw 'Omitted-GenerateMips mutant incorrectly passed'}
    if(!(Select-String -LiteralPath "$out/mutant.stderr.log" -SimpleMatch 'GPU mip pixel/alpha mismatch' -Quiet)){throw 'Mutant failed for an unexpected reason'}
    Write-Output 'PASS: omitted-GenerateMips mutant failed actual lower-mip pixel readback as required'
}
Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) 'GpuMipUpload.h'),"$PSScriptRoot/gpu_mip_warp.cpp","$out/gpu_mip_warp.exe"
