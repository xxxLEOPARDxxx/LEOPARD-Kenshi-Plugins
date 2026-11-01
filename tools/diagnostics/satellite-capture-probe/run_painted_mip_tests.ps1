param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
function Build-Test([string]$header,[string]$name) {
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DPAINTED_MIP_POLICY_HEADER=`"$header`"" "/Fe$out/$name.exe" "/Fo$out/$name.obj" "$PSScriptRoot/painted_mip_tests.cpp"
    if($LASTEXITCODE -ne 0){throw "C++ compilation failed: $name"}
}
Build-Test 'PaintedMipPolicy.h' 'painted_mip_tests'
& "$out/painted_mip_tests.exe" 2>&1 | Tee-Object -FilePath "$out/painted-mip-production.log"
if($LASTEXITCODE -ne 0){throw 'Painted mip/accounting production tests failed'}
if(!$SkipMutation) {
    $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'PaintedMipPolicy.h'))
    $needle='ops.Notify();'
    if(([regex]::Matches($source,[regex]::Escape($needle))).Count -ne 1){throw 'Notify mutation site missing/ambiguous'}
    $mutant=Join-Path $out 'PaintedMipPolicy.omit-notify.h'
    [IO.File]::WriteAllText($mutant,$source.Replace($needle,'/* mutant omits native accounting credit */'),[Text.Encoding]::ASCII)
    Build-Test $mutant.Replace('\','/') 'painted_mip_omit_notify'
    $proc=Start-Process -FilePath "$out/painted_mip_omit_notify.exe" -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput "$out/painted-mip-mutant.stdout.log" -RedirectStandardError "$out/painted-mip-mutant.stderr.log"
    Get-Content -LiteralPath "$out/painted-mip-mutant.stderr.log"
    if($proc.ExitCode -eq 0){throw 'Omitted-notify mutant incorrectly passed'}
    if(!(Select-String -LiteralPath "$out/painted-mip-mutant.stderr.log" -SimpleMatch 'manager credited once and resource Loaded' -Quiet)){throw 'Accounting mutant failed for unexpected reason'}
    Write-Output 'PASS: omitted-notify mutant fails real production completion seam accounting contract'
}
