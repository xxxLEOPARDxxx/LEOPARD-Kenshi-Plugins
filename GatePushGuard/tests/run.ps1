$ErrorActionPreference='Stop'
$taskBuild='D:\DEV\Kenshi\build\GatePushGuard-tests'
New-Item -ItemType Directory -Force -Path $taskBuild | Out-Null
$taskVc='C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC'
$taskSdk='C:\Program Files\Microsoft SDKs\Windows\v7.1'
$env:INCLUDE="$taskVc\include;$taskSdk\Include"
$env:LIB="$taskVc\lib\amd64;$taskSdk\Lib\x64"
& "$taskVc\bin\amd64\cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$taskBuild\core_tests.obj" "/Fe$taskBuild\core_tests.exe" "$PSScriptRoot\core_tests.cpp"
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$taskBuild\core_tests.exe"
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$taskVc\bin\amd64\cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$taskBuild\core_soak.obj" "/Fe$taskBuild\core_soak.exe" "$PSScriptRoot\core_soak.cpp"
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$taskBuild\core_soak.exe"
exit $LASTEXITCODE
