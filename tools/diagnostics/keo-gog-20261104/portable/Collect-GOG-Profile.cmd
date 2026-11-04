@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-GOG-Profile.ps1" -GameDir "%~1"
echo.
echo Diagnostics finished. The report is in the reports folder.
echo This is not a working GOG KEO build.
pause
