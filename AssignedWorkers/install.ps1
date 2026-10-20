# Build AssignedWorkers alone and install it into the game - without the full
# rebuild_all.ps1 cycle.
#
#   powershell -ExecutionPolicy Bypass -File D:\DEV\Kenshi\ours\AssignedWorkers\install.ps1
#
# The locale folder of the mod is replaced, not merged: a catalog left from an
# older version (or a stale .mo, which wins over .po) would hide new strings.
# The player's AssignedWorkers.ini is not touched: the plugin adds new keys to
# it from AssignedWorkers.defaults.ini by itself.
$ErrorActionPreference = "Stop"
$ROOT   = "D:\DEV\Kenshi"
$KENSHI = "D:\SteamLibrary\steamapps\common\Kenshi"
$NAME   = "AssignedWorkers"

if (Get-Process -Name "kenshi_x64", "kenshi" -ErrorAction SilentlyContinue) {
    Write-Host "Close Kenshi first: the game keeps the plugin DLLs locked." -ForegroundColor Red
    exit 1
}

$M = "$KENSHI\mods\$NAME"
if (-not (Test-Path $M)) {
    Write-Host "No mod folder $M - install the mod once with rebuild_all.ps1" -ForegroundColor Red
    exit 1
}

$dll = "$ROOT\build\$NAME\$NAME.dll"
$before = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
& "$ROOT\build.ps1" -SrcDir "$ROOT\ours\$NAME" -Name $NAME
$after = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
if ($after -le $before) {
    Write-Host "BUILD FAILED: $NAME - nothing installed" -ForegroundColor Red
    exit 1
}

Copy-Item $dll $M -Force
Copy-Item "$ROOT\ours\$NAME\README.txt" $M -Force
Copy-Item "$ROOT\ours\$NAME\$NAME.ini" "$M\$NAME.defaults.ini" -Force
if (Test-Path "$M\locale") { Remove-Item "$M\locale" -Recurse -Force }
Copy-Item "$ROOT\ours\$NAME\locale" $M -Recurse -Force
Write-Host "Installed: $NAME" -ForegroundColor Green
