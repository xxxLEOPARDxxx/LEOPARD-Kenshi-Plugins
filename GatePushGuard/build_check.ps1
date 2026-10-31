$ErrorActionPreference='Stop'
$taskPython='C:\Users\User\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& "$PSScriptRoot\tests\run.ps1"
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& 'D:\DEV\Kenshi\ours\build.ps1' -SrcDir $PSScriptRoot -Name GatePushGuard
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $taskPython "$PSScriptRoot\tests\compile_locales.py" --output 'D:\DEV\Kenshi\build\GatePushGuard'
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $taskPython "$PSScriptRoot\tests\audit_native.py" --output 'D:\DEV\Kenshi\build\GatePushGuard-native'
if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $taskPython "$PSScriptRoot\tests\prepare_metadata.py"
exit $LASTEXITCODE
