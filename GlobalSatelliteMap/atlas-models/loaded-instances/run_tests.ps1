param([switch]$SkipMutation)
$ErrorActionPreference='Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
if($SkipMutation){& $python "$PSScriptRoot/run_tests.py" --skip-mutants}else{& $python "$PSScriptRoot/run_tests.py"}
if($LASTEXITCODE -ne 0){throw 'Isolated loaded instance suite failed'}
