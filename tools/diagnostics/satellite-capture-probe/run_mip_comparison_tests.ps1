param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot 'build/mip-comparison'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
function Build-Test([string]$header,[string]$name) {
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W4 "/DMIP_COMPARISON_HEADER=`"$header`"" "/Fe$out/$name.exe" "/Fo$out/$name.obj" "$PSScriptRoot/mip_comparison_warp.cpp" /link d3d11.lib advapi32.lib
    if($LASTEXITCODE -ne 0){throw "Comparison C++ compilation failed: $name"}
}
Build-Test 'MipComparison.h' 'mip_comparison_warp'
& "$out/mip_comparison_warp.exe" 2>&1 | Tee-Object -FilePath "$out/production.log"
if($LASTEXITCODE -ne 0){throw 'Mip comparison production tests failed'}
if(!$SkipMutation) {
    $source=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'MipComparison.h'))
    $mutations=@(
        @{Name='omit-array-copy';Needle='context->CopySubresourceRegion(staging,0,0,0,0,arrayTexture,D3D11CalcSubresource(mip,layer,levels),&box);';Replacement='/* test-only mutant omits array copy */';Expected='one-pixel RGB/alpha corruption must be detected'},
        @{Name='wrong-mip';Needle='D3D11CalcSubresource(mip,layer,levels)';Replacement='D3D11CalcSubresource(0,layer,levels)';Expected='all real GPU mip pixels/channels must match'},
        @{Name='omit-alpha';Needle='if(delta){++result.mismatches[channel];';Replacement='if(delta&&channel!=3){++result.mismatches[channel];';Expected='one-pixel RGB/alpha corruption must be detected'},
        @{Name='omit-cancel-release';Needle='if(Active()){Release();state=Cancelled;error=E_ABORT;}';Replacement='if(Active()){state=Cancelled;error=E_ABORT;}';Expected='cancel releases source/device/context refs'}
    )
    foreach($mutation in $mutations) {
        if(([regex]::Matches($source,[regex]::Escape($mutation.Needle))).Count -ne 1){throw "Missing/ambiguous mutation site: $($mutation.Name)"}
        $header=Join-Path $out "MipComparison.$($mutation.Name).h"
        [IO.File]::WriteAllText($header,$source.Replace($mutation.Needle,$mutation.Replacement),[Text.Encoding]::ASCII)
        $exe="mip_comparison_$($mutation.Name)"
        Build-Test $header.Replace('\','/') $exe
        $proc=Start-Process -FilePath "$out/$exe.exe" -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput "$out/$exe.stdout.log" -RedirectStandardError "$out/$exe.stderr.log"
        Get-Content -LiteralPath "$out/$exe.stderr.log"
        if($proc.ExitCode -eq 0){throw "Mutant incorrectly passed: $($mutation.Name)"}
        if(!(Select-String -LiteralPath "$out/$exe.stderr.log" -SimpleMatch $mutation.Expected -Quiet)){throw "Mutant failed unexpectedly: $($mutation.Name)"}
        Write-Output "PASS: $($mutation.Name) mutant rejected by real GPU readback"
    }
}
Get-FileHash -Algorithm SHA256 -LiteralPath "$PSScriptRoot/MipComparison.h","$PSScriptRoot/MipComparisonPolicy.h","$out/mip_comparison_warp.exe"
