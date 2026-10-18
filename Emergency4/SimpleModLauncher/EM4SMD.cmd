@echo off
rem EM4SMD - Emergency 4 Simple Mod Launcher 1.0
setlocal DisableDelayedExpansion
set "EM4_MOD_LAUNCHER=%~f0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "& ([scriptblock]::Create(([IO.File]::ReadAllText($env:EM4_MOD_LAUNCHER) -split '(?m)^:__POWERSHELL__\r?$',2)[1]))"
exit /b %errorlevel%
:__POWERSHELL__
$ErrorActionPreference = 'Stop'
try {
    $game = Split-Path -Parent $env:EM4_MOD_LAUNCHER
    if (!(Test-Path -LiteralPath (Join-Path $game 'Em4Deluxe.exe'))) {
        $parent = Split-Path -Parent $game
        if (Test-Path -LiteralPath (Join-Path $parent 'Em4Deluxe.exe')) { $game = $parent }
    }
    $exe = Join-Path $game 'Em4Deluxe.exe'
    $folder = Join-Path $game 'Mods'
    if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Em4Deluxe.exe was not found. Place this launcher in the game folder or a direct subfolder.' }
    if (!(Test-Path -LiteralPath $folder -PathType Container)) { throw 'The Mods folder was not found.' }
    $mods = @(Get-ChildItem -LiteralPath $folder -Directory | Sort-Object Name)
    if (!$mods.Count) { throw 'No mod folders were found in Mods.' }

    Write-Host ''
    Write-Host 'Emergency 4 Simple Mod Launcher'
    Write-Host ''
    for ($i = 0; $i -lt $mods.Count; $i++) { Write-Host ('{0,2} - {1}' -f ($i + 1), $mods[$i].Name) }
    Write-Host ' 0 - Exit'
    Write-Host ''
    do {
        $answer = Read-Host 'Select a mod number'
        $number = 0
        $valid = [int]::TryParse($answer, [ref]$number) -and $number -ge 0 -and $number -le $mods.Count
        if (!$valid) { Write-Host 'Invalid selection. Enter one of the listed numbers.' }
    } until ($valid)
    if ($number -eq 0) { exit 0 }

    $name = $mods[$number - 1].Name
    Write-Host ('Launching: ' + $name)
    Start-Process -FilePath $exe -WorkingDirectory $game -ArgumentList @('-game', '-mod', ('"' + $name + '"'))
    exit 0
} catch {
    Write-Host ('Error: ' + $_.Exception.Message) -ForegroundColor Red
    $null = Read-Host 'Press Enter to close'
    exit 1
}
