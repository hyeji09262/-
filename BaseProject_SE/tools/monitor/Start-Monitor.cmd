@echo off
setlocal
title Starlight World Monitor
echo Starting the local monitor. Keep this window open.
echo Address: http://127.0.0.1:8765/
echo.
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -NoExit -ExecutionPolicy Bypass -File "%~dp0Start-Monitor.ps1"
echo.
echo Monitor launcher has stopped. Any error is shown above.
pause
endlocal
