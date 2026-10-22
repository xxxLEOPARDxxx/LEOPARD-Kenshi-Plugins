# Сборка и запуск тестов ядра Dynamic Factions (core/ - без заголовков игры).
# Окружение VC10 - как в ours/build.ps1. Код возврата - число проваленных проверок.
$ErrorActionPreference = "Stop"
$VC  = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC"
$SDK = "C:\Program Files\Microsoft SDKs\Windows\v7.1"
$root = Split-Path $PSScriptRoot -Parent
$out = "D:\DEV\Kenshi\build\DynamicFactionsTests"
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem $out -Filter "*.obj" -ErrorAction SilentlyContinue | Remove-Item -Force
$env:INCLUDE = @("$VC\include", "$SDK\Include", $root) -join ";"
$env:LIB     = @("$VC\lib\amd64", "$SDK\Lib\x64") -join ";"
$env:PATH    = "$VC\bin\amd64;$env:PATH"
$cpps = @(Get-ChildItem (Join-Path $root "core") -Filter "*.cpp" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
$cpps += (Join-Path $PSScriptRoot "core_tests.cpp")
& "$VC\bin\amd64\cl.exe" /nologo /EHsc /MD /O2 /W3 "/Fo$out\" "/Fe$out\core_tests.exe" $cpps
if ($LASTEXITCODE -ne 0) { Write-Host "COMPILE FAILED"; exit 100 }
& "$out\core_tests.exe"
exit $LASTEXITCODE
