$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$repo=Split-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) -Parent
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$kl='D:/DEV/Kenshi/KenshiLib-src/Include'
$boost='D:/DEV/Kenshi/deps/boost_1_60_0'
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $out | Out-Null
$env:INCLUDE="$vc/include;$sdk/Include;$kl;$kl/ogre;$boost;D:/DEV/Kenshi/ours/shared;$PSScriptRoot"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;$boost/stage/lib"
$env:PATH="$vc/bin/amd64;$env:PATH"
& "$vc/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GS- /GL /W3 /D NDEBUG /D WIN32 /D _WINDOWS /D _USRDLL /D UNICODE /D _UNICODE "/Fo$out/CaptureProbe.obj" "$PSScriptRoot/CaptureProbe.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/link.exe" /nologo /DLL /MACHINE:X64 /LTCG "/OUT:$out/SatelliteCaptureProbe.dll" KenshiLib.lib OgreMain_x64.lib MyGUIEngine_x64.lib user32.lib kernel32.lib advapi32.lib "$out/CaptureProbe.obj"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python (Join-Path $repo 'GlobalSatelliteMap/tests/test_pe_contract.py') "$out/SatelliteCaptureProbe.dll" --imports-root 'D:/SteamLibrary/steamapps/common/Kenshi' --report "$PSScriptRoot/loader-contract.json"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/policy_tests.exe" "/Fo$out/policy_tests.obj" "$PSScriptRoot/policy_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/policy_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/attachment_tests.exe" "/Fo$out/attachment_tests.obj" "$PSScriptRoot/attachment_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/attachment_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fe$out/pose_tests.exe" "/Fo$out/pose_tests.obj" "$PSScriptRoot/pose_tests.cpp"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& "$out/pose_tests.exe"
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
& $python "$PSScriptRoot/source_contracts.py"
exit $LASTEXITCODE
