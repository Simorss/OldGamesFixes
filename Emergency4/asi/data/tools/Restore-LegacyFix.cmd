@echo off
setlocal
echo Restore a previous script-based Emergency 4 fix before migrating to ASI.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0legacy\Emergency4-Fix.ps1" -Restore
echo.
pause
exit /b %errorlevel%
