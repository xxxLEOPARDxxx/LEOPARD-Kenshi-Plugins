$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$kl='D:/DEV/Kenshi/KenshiLib-src/Include'
$out=Join-Path $PSScriptRoot '_generated/origin-proof'
New-Item -ItemType Directory -Force $out | Out-Null
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/audit_origin_proof.py"
if($LASTEXITCODE -ne 0){throw 'Scalar origin proof source audit failed'}
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared;$PSScriptRoot;$PSScriptRoot/.."
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GL /W1 /DUNICODE /D_UNICODE "/Fo$out/adapter.obj" "$PSScriptRoot/origin_proof_compile.cpp" *> "$out/compile.log"
if($LASTEXITCODE -ne 0){Get-Content "$out/compile.log" -Tail 12;throw 'Scalar origin proof compile failed'}
'Scalar origin proof VC10 compile PASS (not linked/invoked)'
