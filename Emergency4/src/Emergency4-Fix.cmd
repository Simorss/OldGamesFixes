@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Emergency4-Fix.ps1" %*
set "fixExitCode=%ERRORLEVEL%"
echo.
if not "%fixExitCode%"=="0" echo The operation did not complete. Read the message above.
pause
exit /b %fixExitCode%
