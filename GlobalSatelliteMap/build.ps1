# Scoped development build: native linking alone does not prove RE_Kenshi can initialize us.
$ErrorActionPreference = 'Stop'
$python='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& $python (Join-Path (Split-Path $PSScriptRoot -Parent) 'tools/po2mo.py') `
    (Join-Path $PSScriptRoot 'locale/en_GB/LC_MESSAGES/global_satellite_map.po') `
    (Join-Path $PSScriptRoot 'locale/ru_RU/LC_MESSAGES/global_satellite_map.po') `
    (Join-Path $PSScriptRoot 'locale/zh_CN/LC_MESSAGES/global_satellite_map.po')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $python (Join-Path $PSScriptRoot 'generate_viewer_assets.py')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path (Split-Path $PSScriptRoot -Parent) 'build.ps1') -SrcDir $PSScriptRoot -Name GlobalSatelliteMap
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$dll = 'D:/DEV/Kenshi/build/GlobalSatelliteMap/GlobalSatelliteMap.dll'
& $python (Join-Path $PSScriptRoot 'tests/test_pe_contract.py') $dll --mcm --imports-root 'D:/SteamLibrary/steamapps/common/Kenshi'
exit $LASTEXITCODE
