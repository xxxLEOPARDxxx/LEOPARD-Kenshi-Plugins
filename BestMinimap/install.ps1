# Build BestMinimap alone and install it into the game - without the full
# rebuild_all.ps1 cycle.
#
#   powershell -ExecutionPolicy Bypass -File D:\DEV\Kenshi\ours\BestMinimap\install.ps1
#   ... install.ps1 -WithWantedMap   also rebuild and install WantedMap
#                                    (its WantedMap_GetMarkers feeds the minimap)
#
# The locale folder of the mod is replaced, not merged: a catalog left from an
# older version (or a stale .mo, which wins over .po) would hide new strings.
param([switch]$WithWantedMap)
$ErrorActionPreference = "Stop"
$ROOT   = "D:\DEV\Kenshi"
$KENSHI = "D:\SteamLibrary\steamapps\common\Kenshi"

if (Get-Process -Name "kenshi_x64", "kenshi" -ErrorAction SilentlyContinue) {
    Write-Host "Close Kenshi first: the game keeps the plugin DLLs locked." -ForegroundColor Red
    exit 1
}

function Build($name) {
    $dll = "$ROOT\build\$name\$name.dll"
    $before = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
    & "$ROOT\build.ps1" -SrcDir "$ROOT\ours\$name" -Name $name
    $after = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
    if ($after -le $before) {
        Write-Host "BUILD FAILED: $name - nothing installed" -ForegroundColor Red
        exit 1
    }
}

Build "BestMinimap"
$M = "$KENSHI\mods\BestMinimap"
if (-not (Test-Path $M)) {
    New-Item -ItemType Directory -Force -Path $M | Out-Null
    if (Test-Path "$KENSHI\mods\RecruitMarkers\RecruitMarkers.mod") {
        Copy-Item "$KENSHI\mods\RecruitMarkers\RecruitMarkers.mod" "$M\BestMinimap.mod" -Force
    }
    Write-Host "Mod folder created - enable BestMinimap in the launcher" -ForegroundColor Yellow
}
'{ "Plugins": [ "BestMinimap.dll" ] }' | Set-Content "$M\RE_Kenshi.json" -Encoding ASCII
Copy-Item "$ROOT\build\BestMinimap\BestMinimap.dll" $M -Force
Copy-Item "$ROOT\ours\BestMinimap\README.txt" $M -Force
Copy-Item "$ROOT\ours\BestMinimap\BestMinimap.ini" "$M\BestMinimap.defaults.ini" -Force
if (Test-Path "$M\locale") { Remove-Item "$M\locale" -Recurse -Force }
Copy-Item "$ROOT\ours\BestMinimap\locale" $M -Recurse -Force
Write-Host "Installed: BestMinimap" -ForegroundColor Green

if ($WithWantedMap) {
    Build "WantedMap"
    Copy-Item "$ROOT\build\WantedMap\WantedMap.dll" "$KENSHI\mods\WantedMap" -Force
    Write-Host "Installed: WantedMap" -ForegroundColor Green
}
