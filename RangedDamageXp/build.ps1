# Run in the extracted corresponding-source bundle.
param(
    [Parameter(Mandatory=$true)][string]$VcDir,
    [Parameter(Mandatory=$true)][string]$WindowsSdkDir,
    [Parameter(Mandatory=$true)][string]$BoostDir,
    [Parameter(Mandatory=$true)][string]$ImportLibDir,
    [string]$OutDir = "$PSScriptRoot/build"
)
$ErrorActionPreference = 'Stop'
$bundle = Split-Path $PSScriptRoot -Parent
$headers = Join-Path $bundle 'KenshiLib/Include'
$env:INCLUDE = @("$VcDir/include", "$WindowsSdkDir/Include", $headers, "$headers/ogre",
                 $BoostDir, "$bundle/shared", $PSScriptRoot) -join ';'
$env:LIB = @("$VcDir/lib/amd64", "$WindowsSdkDir/Lib/x64", $ImportLibDir, "$BoostDir/stage/lib") -join ';'
$OutDir = (New-Item -ItemType Directory -Force -Path $OutDir).FullName
$cpp = @("$PSScriptRoot/RangedDamageXp.cpp", "$PSScriptRoot/BoltTrails.cpp")
& "$VcDir/bin/amd64/cl.exe" /nologo /c /EHsc /MD /O2 /GS- /GL /W1 /DNDEBUG /DWIN32 /D_WINDOWS /D_USRDLL /DUNICODE /D_UNICODE "/Fo$OutDir\" $cpp
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& "$VcDir/bin/amd64/link.exe" /nologo /DLL /MACHINE:X64 /LTCG "/OUT:$OutDir/RangedDamageXp.dll" `
    KenshiLib.lib OgreMain_x64.lib user32.lib kernel32.lib "$OutDir/RangedDamageXp.obj" "$OutDir/BoltTrails.obj"
if ($LASTEXITCODE -ne 0) { throw 'Linking failed' }
