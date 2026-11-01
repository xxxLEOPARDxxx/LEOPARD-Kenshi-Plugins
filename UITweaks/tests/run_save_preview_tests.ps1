$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$out=Join-Path $PSScriptRoot '_generated'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$env:PATH="$vc/bin/amd64;$env:PATH"
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/save_preview_policy_test.exe" "/Fo$out/save_preview_policy_test.obj" "$PSScriptRoot/save_preview_policy_test.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/save_preview_policy_test.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/save_preview_codec_test.exe" "/Fo$out/save_preview_codec_test.obj" "$PSScriptRoot/save_preview_codec_test.cpp" oleaut32.lib
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/save_preview_codec_test.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/save_preview_gpu_test.exe" "/Fo$out/save_preview_gpu_test.obj" "$PSScriptRoot/save_preview_gpu_test.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/save_preview_gpu_test.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_writer.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_lifecycle.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_alpha.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_selection.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_capture.py"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/test_save_preview_text.py"
exit $LASTEXITCODE
