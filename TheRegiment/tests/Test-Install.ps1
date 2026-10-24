[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference='Stop'
$stage=Split-Path -Parent $PSScriptRoot
$fixture=Join-Path $stage 'build/install-fixture'
$system=Join-Path $fixture 'system'
New-Item -ItemType Directory -Force -Path $system | Out-Null
$privateExe=Join-Path $system 'sas.exe'
if (Test-Path -LiteralPath $privateExe) { throw 'Fixture already exists; inspect it before rerunning.' }
Copy-Item -LiteralPath (Join-Path $GameDirectory 'system/sas.exe') -Destination $privateExe
try {
    & (Join-Path $stage 'tools/Install.ps1') -GameDirectory $fixture
    foreach($name in @('dinput8.dll','scripts/TheRegiment.FOVFix.asi','scripts/TheRegiment.FOVFix.ini','scripts/ThirteenAG-LICENSE.txt','TheRegiment.FOVFix.install.json')) { if (!(Test-Path -LiteralPath (Join-Path $system $name))) { throw "Missing installed file: $name" } }
    $blocked=$false
    try { & (Join-Path $stage 'tools/Install.ps1') -GameDirectory $fixture } catch { $blocked=$true }
    if (!$blocked) { throw 'Duplicate installation was not blocked.' }
    & (Join-Path $stage 'tools/Uninstall.ps1') -GameDirectory $fixture
    foreach($name in @('dinput8.dll','scripts/TheRegiment.FOVFix.asi','scripts/TheRegiment.FOVFix.ini','scripts/ThirteenAG-LICENSE.txt','TheRegiment.FOVFix.install.json')) { if (Test-Path -LiteralPath (Join-Path $system $name)) { throw "Uninstall left an owned file: $name" } }
    Copy-Item -LiteralPath (Join-Path $stage 'third-party/Ultimate-ASI-Loader/dinput8.dll') -Destination (Join-Path $system 'dinput8.dll')
    & (Join-Path $stage 'tools/Install.ps1') -GameDirectory $fixture
    & (Join-Path $stage 'tools/Uninstall.ps1') -GameDirectory $fixture
    if (!(Test-Path -LiteralPath (Join-Path $system 'dinput8.dll'))) { throw 'An existing loader was removed.' }
    Remove-Item -LiteralPath (Join-Path $system 'dinput8.dll')
    [IO.File]::WriteAllText((Join-Path $system 'dinput8.dll'),'unrelated existing DLL')
    $blocked=$false
    try { & (Join-Path $stage 'tools/Install.ps1') -GameDirectory $fixture } catch { $blocked=$true }
    if (!$blocked -or [IO.File]::ReadAllText((Join-Path $system 'dinput8.dll')) -ne 'unrelated existing DLL') { throw 'Unrelated loader was not protected.' }
    if (Test-Path -LiteralPath (Join-Path $system 'scripts/TheRegiment.FOVFix.asi')) { throw 'Conflict caused a partial install.' }
    Remove-Item -LiteralPath (Join-Path $system 'dinput8.dll')
    Write-Output 'PASS: fresh install, duplicate rejection, uninstall, existing-loader preservation, unrelated-loader conflict.'
} finally {
    $resolved=[IO.Path]::GetFullPath($privateExe)
    $expected=[IO.Path]::GetFullPath((Join-Path $stage 'build/install-fixture/system/sas.exe'))
    if ($resolved -ne $expected -or !$resolved.StartsWith([IO.Path]::GetFullPath((Join-Path $stage 'build'))+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Private fixture cleanup path check failed.' }
    if (Test-Path -LiteralPath $privateExe) { Remove-Item -LiteralPath $privateExe -Force }
}
