# Build Kenshi plugins with the VC10 (VS2010) compiler directly, bypassing MSBuild.
# Usage:  .\build.ps1 -SrcDir <source folder> -Name <PluginName>
#         [-SubDirs src,...] [-Include dir,...] [-Define NAME=1,...] [-FromVcxproj]
# -FromVcxproj: список .cpp и пути заголовков $(ProjectDir)... берутся из
# <SrcDir>\*.vcxproj (проекты Emkej), вместо обхода папки.
# SubDirs/Include/Define нужны проектам Emkej: у них исходники в src\, а
# заголовки Mod Hub - в include\emc или tools\mod-hub-sdk\include. Пути -
# относительно SrcDir.
param(
    [Parameter(Mandatory=$true)][string]$SrcDir,
    [Parameter(Mandatory=$true)][string]$Name,
    [string[]]$SubDirs = @(),
    [string[]]$Include = @(),
    [string[]]$Define = @(),
    [switch]$FromVcxproj
)
$ErrorActionPreference = "Stop"

$VC = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC"

# Windows SDK 7.1 is the matching pair for VC10.
# SDK 10 headers use SAL annotations VC10 cannot parse (driverspecs.h errors).
$SDK = "C:\Program Files\Microsoft SDKs\Windows\v7.1"

# Headers come from the KenshiLib source repo, NOT from the _deps copy:
# the deps copy is older (no AI/Blackboard.h, enums duplicated across headers).
# The .lib comes from the KenshiLib v0.2.1 release, which matches those headers.
$KLINC = "D:\DEV\Kenshi\KenshiLib-src\Include"
$KLLIB = "D:\DEV\Kenshi\deps\KenshiLib\Libraries"

function EnvOr([string]$n, [string]$fallback) {
    $v = [Environment]::GetEnvironmentVariable($n, "User")
    if ([string]::IsNullOrWhiteSpace($v)) { return $fallback } else { return $v }
}
$BST = EnvOr "BOOST_INCLUDE_PATH" "D:\DEV\Kenshi\deps\boost_1_60_0"

$out = "D:\DEV\Kenshi\build\$Name"
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem $out -Filter "*.obj" -ErrorAction SilentlyContinue | Remove-Item -Force

# 'ogre' subdir is needed too: headers under ogre/Math/... include "OgreConfig.h"
# by plain name, which only resolves if ogre/ itself is on the include path.
# Sibling plugin folders are on the include path too: Au2942 shares headers
# between plugins (SquadAutonomy includes MoreImmersiveBars/lektorExtension.h).
$siblings = @(Get-ChildItem (Split-Path $SrcDir -Parent) -Directory |
              ForEach-Object { $_.FullName })
# shared\ — наш общий код на все плагины (сейчас там Localization.h)
$vcxCpps = @()
if ($FromVcxproj) {
    $proj = Get-ChildItem $SrcDir -Filter "*.vcxproj" | Select-Object -First 1
    $xml = Get-Content $proj.FullName -Raw
    $vcxCpps = @([regex]::Matches($xml, '<ClCompile Include="([^"]+\.cpp)"') |
                 ForEach-Object { Join-Path $SrcDir $_.Groups[1].Value })
    # $(ProjectDir)include;$(ProjectDir)tools\mod-hub-sdk\include;...
    $m = [regex]::Match($xml, '<IncludePath>([^<]*)</IncludePath>')
    if ($m.Success) {
        $Include += @($m.Groups[1].Value.Split(';') |
                      Where-Object { $_ -like '$(ProjectDir)*' } |
                      ForEach-Object { $_.Substring(13).TrimEnd('\') } |
                      Where-Object { $_ -ne '' })
    }
}
$extraInc = @($Include | Select-Object -Unique | ForEach-Object { Join-Path $SrcDir $_ })
$env:INCLUDE = (@("$VC\include", "$SDK\Include", $KLINC, "$KLINC\ogre", $BST,
                  "D:\DEV\Kenshi\shared", $SrcDir) + $extraInc + $siblings) -join ";"
$BLIB = Join-Path $BST "stage\lib"   # prebuilt boost libs (libboost_thread-vc100-...)
$env:LIB     = @("$VC\lib\amd64", "$SDK\Lib\x64", $KLLIB, $BLIB) -join ";"
$env:PATH    = "$VC\bin\amd64;$env:PATH"

$cpps = @(Get-ChildItem $SrcDir -Filter "*.cpp" | ForEach-Object { $_.FullName })
if ($FromVcxproj) { $cpps = $vcxCpps }
foreach ($d in $SubDirs) {
    $cpps += @(Get-ChildItem (Join-Path $SrcDir $d) -Filter "*.cpp" | ForEach-Object { $_.FullName })
}
Write-Host ("Compiling {0} file(s)..." -f $cpps.Count) -ForegroundColor Cyan

# UNICODE is required: the examples build with CharacterSet=Unicode, and KenshiLib
# headers pass L"" literals to CreateMutex etc.
# /GL (whole program optimization) is REQUIRED, not an optimization choice:
# without it MSVC emits a local thunk for imported member functions, so
# KenshiLib::GetRealAddress(&Class::Method) receives an address inside our own
# DLL and asserts "Incorrect address in KenshiLib::GetRealAddress()".
# Pairs with /LTCG at link time.
$clArgs = @("/nologo","/c","/EHsc","/MD","/O2","/GS-","/GL","/W1",
            "/D","NDEBUG","/D","WIN32","/D","_WINDOWS","/D","_USRDLL",
            "/D","UNICODE","/D","_UNICODE",
            "/Fo$out\") + @($Define | ForEach-Object { "/D$_" }) + $cpps
& "$VC\bin\amd64\cl.exe" $clArgs
if ($LASTEXITCODE -ne 0) { Write-Host "COMPILE FAILED" -ForegroundColor Red; exit 1 }

$objs = @(Get-ChildItem $out -Filter "*.obj" | ForEach-Object { $_.FullName })
Write-Host ("Linking {0} obj..." -f $objs.Count) -ForegroundColor Cyan
$linkArgs = @("/nologo","/DLL","/MACHINE:X64","/LTCG","/OUT:$out\$Name.dll",
              "KenshiLib.lib","OgreMain_x64.lib","MyGUIEngine_x64.lib",
              "user32.lib","kernel32.lib",
              # gdi32 нужен BetterLooting: он рисует своё окно настроек
              # средствами Win32 GDI, а не через MyGUI. Остальным плагинам
              # лишняя библиотека в списке не мешает - линкер возьмёт из неё
              # только то, на что есть ссылки.
              "gdi32.lib") + $objs
& "$VC\bin\amd64\link.exe" $linkArgs
if ($LASTEXITCODE -ne 0) { Write-Host "LINK FAILED" -ForegroundColor Red; exit 1 }

Write-Host "OK: $out\$Name.dll" -ForegroundColor Green
Get-Item "$out\$Name.dll" | Select-Object Name, Length | Format-Table -AutoSize
