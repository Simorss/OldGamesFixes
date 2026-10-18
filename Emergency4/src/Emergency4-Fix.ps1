# OldGamesFixes / Emergency 4 - 1.1.0-beta.1
# Windows PowerShell 5.1 or PowerShell 7. No game launch or runtime download.
[CmdletBinding()]
param(
    [string]$GamePath,
    [string]$Executable,
    [string]$Config,
    [int]$Width = 0,
    [int]$Height = 0,
    [ValidateSet('Original','Stretched')][string]$InterfaceMode,
    [switch]$Restore,
    [switch]$Preview,
    [switch]$NonInteractive
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$version = '1.1.0-beta.1'
$culture = [Globalization.CultureInfo]::InvariantCulture

function Get-ByteHash([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-','') }
    finally { $sha.Dispose() }
}
function Get-FileSha([string]$Path) { return Get-ByteHash ([IO.File]::ReadAllBytes($Path)) }
function Read-Config([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $encoding = [Text.Encoding]::GetEncoding(1252)
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 255 -and $bytes[1] -eq 254) { $encoding = [Text.Encoding]::Unicode }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 254 -and $bytes[1] -eq 255) { $encoding = [Text.Encoding]::BigEndianUnicode }
    elseif ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) { $encoding = [Text.Encoding]::UTF8 }
    return @{ Bytes=$bytes; Encoding=$encoding; Text=$encoding.GetString($bytes) }
}
function Get-ConfigValue([string]$Text,[string]$Key) {
    $pattern = '<var\s+name="'+[regex]::Escape($Key)+'"\s+value="([^"]*)"\s*/>'
    $matches = [regex]::Matches($Text,$pattern)
    if ($matches.Count -ne 1) { throw "Configuration value '$Key' is missing or duplicated." }
    return $matches[0].Groups[1].Value
}
function Set-ConfigValue([string]$Text,[string]$Key,[string]$Value) {
    $null = Get-ConfigValue $Text $Key
    $pattern = '(<var\s+name="'+[regex]::Escape($Key)+'"\s+value=")[^"]*("\s*/>)'
    return [regex]::Replace($Text,$pattern,('${1}'+$Value+'${2}'))
}
function Write-Atomic([string]$Path,[byte[]]$Bytes) {
    $temp = $Path+'.ogf-'+[Guid]::NewGuid().ToString('N')+'.tmp'
    try {
        [IO.File]::WriteAllBytes($temp,$Bytes)
        if (Test-Path -LiteralPath $Path) { [IO.File]::Replace($temp,$Path,[NullString]::Value) }
        else { [IO.File]::Move($temp,$Path) }
    } finally { if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force } }
}
function Game-File([string]$Name) {
    if (!$Name -or [IO.Path]::GetFileName($Name) -ne $Name -or $Name.Contains(':') -or $Name -in @('.','..')) {
        throw 'Specify a filename located directly inside the game folder.'
    }
    return Join-Path $GamePath $Name
}
function Select-One($Candidates,[string]$Label) {
    $items = @($Candidates)
    if ($items.Count -eq 0) { throw "No compatible $Label found. Specify the correct game folder or filename." }
    if ($items.Count -eq 1) { return $items[0] }
    if ($NonInteractive) { throw "Multiple $Label candidates found. Specify -Executable or -Config explicitly." }
    Write-Host "Choose the $Label file:"
    for ($i=0;$i -lt $items.Count;$i++) { Write-Host (('{0} - {1}' -f ($i+1),$items[$i])) }
    do { $answer=Read-Host 'Selection'; $number=0 } until ([int]::TryParse($answer,[ref]$number) -and $number -ge 1 -and $number -le $items.Count)
    return $items[$number-1]
}
function Inspect-Engine([byte[]]$Bytes) {
    try { return [OldGamesFixesV2.Engine]::Inspect($Bytes) }
    catch { throw ('Engine recognition failed. '+$_.Exception.GetBaseException().Message) }
}

if (!$NonInteractive -and !$Restore -and !$Preview -and $Width -eq 0 -and $Height -eq 0 -and !$InterfaceMode) {
    Write-Host "Emergency 4 Resolution Fix $version"
    Write-Host '1 - Apply / change resolution and interface'
    Write-Host '2 - Restore the files saved before first use'
    Write-Host '0 - Exit'
    do { $action=Read-Host 'Selection [1]'; if (!$action) { $action='1' } } until ($action -in @('0','1','2'))
    if ($action -eq '0') { return }
    if ($action -eq '2') { $Restore=$true }
}
if (!$GamePath) {
    if (@(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.exe' -File).Count) { $GamePath=$PSScriptRoot }
    elseif ($NonInteractive) { throw 'Specify -GamePath.' }
    else { $GamePath=(Read-Host 'Game installation folder').Trim().Trim('"') }
}
$GamePath = (Resolve-Path -LiteralPath $GamePath).ProviderPath
$backupDir = Join-Path $GamePath 'OldGamesFixes-Emergency4-Backup'
$statePath = Join-Path $backupDir 'state.json'
$state = $null; $oldSchema=$false
if (Test-Path -LiteralPath $backupDir) {
    if (!(Test-Path -LiteralPath $statePath -PathType Leaf)) { throw 'The backup is incomplete and will not be overwritten.' }
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    if ($state.Schema -eq 'Emergency4ResolutionFix/1') {
        $oldSchema=$true
        if (!$Executable) { $Executable='Em4Deluxe.exe' }
        if (!$Config) { $Config='em4deluxe.cfg' }
    } elseif ($state.Schema -eq 'Emergency4ResolutionFix/2') {
        if ($Executable -and $Executable -ne $state.ExecutableName) { throw 'The existing backup belongs to a different executable.' }
        if ($Config -and $Config -ne $state.ConfigName) { throw 'The existing backup belongs to a different configuration file.' }
        $Executable=$state.ExecutableName; $Config=$state.ConfigName
    } else { throw 'Unknown backup format.' }
}
if (!('OldGamesFixesV2.Engine' -as [type])) { Add-Type -Path (Join-Path $PSScriptRoot 'EnginePatch.cs') }
if (!$Executable) {
    $candidates = @()
    foreach ($file in Get-ChildItem -LiteralPath $GamePath -Filter '*.exe' -File) {
        try { $null=[OldGamesFixesV2.Engine]::Inspect([IO.File]::ReadAllBytes($file.FullName)); $candidates += $file.Name }
        catch { } # Launchers and unrelated utilities are not game engine candidates.
    }
    $Executable=Select-One $candidates 'executable'
}
$exe = Game-File $Executable
if ([IO.Path]::GetExtension($Executable) -ine '.exe') { throw 'Select a Windows .exe game executable.' }
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'The selected executable does not exist.' }
$processName=[IO.Path]::GetFileNameWithoutExtension($Executable)
if (Get-Process -Name $processName -ErrorAction SilentlyContinue) { throw 'Close the game before continuing.' }
if (!$Config) {
    $candidates = @()
    foreach ($file in Get-ChildItem -LiteralPath $GamePath -Filter '*.cfg' -File) {
        try {
            $candidate=Read-Config $file.FullName
            foreach ($key in @('r_xres','r_yres','r_fov')) { $null=Get-ConfigValue $candidate.Text $key }
            $candidates += $file.Name
        } catch { }
    }
    $matching=$processName+'.cfg'
    if ($candidates -contains $matching) { $Config=$matching }
    else { $Config=Select-One $candidates 'configuration' }
}
$cfg=Game-File $Config
if ([IO.Path]::GetExtension($Config) -ine '.cfg') { throw 'Select a .cfg game configuration file.' }
if (!(Test-Path -LiteralPath $cfg -PathType Leaf)) { throw 'The selected configuration file does not exist.' }
$backupExe=Join-Path $backupDir $Executable
$backupCfg=Join-Path $backupDir $Config
$baselinePath=Join-Path $backupDir 'engine-baseline.bin'
if ($state) {
    foreach ($file in @($backupExe,$backupCfg)) { if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw 'Backup files are missing.' } }
    if ((Get-FileSha $backupExe) -ne $state.BackupExeSha256 -or (Get-FileSha $backupCfg) -ne $state.BackupCfgSha256) { throw 'Backup integrity verification failed.' }
}
$currentExe=[IO.File]::ReadAllBytes($exe); $currentHash=Get-ByteHash $currentExe
$configData=Read-Config $cfg
foreach ($key in @('r_xres','r_yres','r_fov')) { $null=Get-ConfigValue $configData.Text $key }
if ($state -and $currentHash -notin @($state.InstalledExeSha256,$state.BackupExeSha256)) {
    throw 'The executable changed after installation. Preserve those changes before reapplying or restoring this fix.'
}
if ($Restore) {
    if (!$state) { throw 'No backup from this patcher was found.' }
    if ($Preview) { Write-Output "Would restore the backup in $backupDir"; return }
    $recovery=Join-Path $backupDir ('config-before-restore-'+[Guid]::NewGuid().ToString('N')+'.cfg')
    [IO.File]::WriteAllBytes($recovery,$configData.Bytes)
    try { Write-Atomic $exe ([IO.File]::ReadAllBytes($backupExe)); Write-Atomic $cfg ([IO.File]::ReadAllBytes($backupCfg)) }
    catch { Write-Atomic $exe $currentExe; Write-Atomic $cfg $configData.Bytes; throw }
    Write-Output 'Restored the executable and configuration saved before first use.'
    return
}

if ($state -and !$oldSchema) {
    if (!(Test-Path -LiteralPath $baselinePath) -or (Get-FileSha $baselinePath) -ne $state.BaselineSha256) { throw 'The engine baseline backup is missing or damaged.' }
    $layout=Inspect-Engine ([IO.File]::ReadAllBytes($baselinePath))
} elseif ($state) { $layout=Inspect-Engine ([IO.File]::ReadAllBytes($backupExe)) }
else { $layout=Inspect-Engine $currentExe }
$currentW=Get-ConfigValue $configData.Text 'r_xres'; $currentH=Get-ConfigValue $configData.Text 'r_yres'
if ($Width -eq 0 -or $Height -eq 0) {
    if ($NonInteractive) { throw 'Specify -Width and -Height.' }
    do {
        $resolution=Read-Host "Resolution, for example 3440x1440 [$($currentW)x$($currentH)]"
        if (!$resolution) { $resolution=$currentW+'x'+$currentH }
        $match=[regex]::Match($resolution,'^\s*(\d{3,5})\s*[xX*]\s*(\d{3,5})\s*$')
        if ($match.Success) { $Width=[int]$match.Groups[1].Value; $Height=[int]$match.Groups[2].Value }
    } until ($match.Success -and $Width -ge 320 -and $Width -le 16384 -and $Height -ge 200 -and $Height -le 16384)
}
if ($Width -lt 320 -or $Width -gt 16384 -or $Height -lt 200 -or $Height -gt 16384) { throw 'Allowed dimensions: width 320..16384, height 200..16384.' }
if (!$InterfaceMode) {
    if ($NonInteractive) { throw 'Specify -InterfaceMode Original or Stretched.' }
    Write-Host '1 - Original 4:3 menu and HUD proportions, centered'
    Write-Host '2 - Full-screen stretched menu and HUD'
    do { $choice=Read-Host 'Interface [1]'; if (!$choice) { $choice='1' } } until ($choice -in @('1','2'))
    if ($choice -eq '1') { $InterfaceMode='Original' } else { $InterfaceMode='Stretched' }
}
$baseFov=[double]::Parse((Get-ConfigValue $configData.Text 'r_fov'),$culture)
if ($state) { $baseFov=[double]$state.OriginalFocalLength }
elseif ([Math]::Abs($layout.PreviousAspect-(4.0/3.0)) -gt 0.000001) {
    $baseFov=[Math]::Round($baseFov*$layout.PreviousAspect/(4.0/3.0),4)
    # Recover the exact pre-fix focal value when the earlier fixed-resolution backup is available.
    $legacyExe=Join-Path $GamePath ('UltrawideFix-Backup\'+$Executable)
    $legacyCfg=Join-Path $GamePath ('UltrawideFix-Backup\'+$Config)
    if ((Test-Path -LiteralPath $legacyExe) -and (Test-Path -LiteralPath $legacyCfg)) {
        try {
            $legacy=Inspect-Engine ([IO.File]::ReadAllBytes($legacyExe))
            if ((Get-ByteHash $legacy.Baseline) -eq (Get-ByteHash $layout.Baseline)) {
                $baseFov=[double]::Parse((Get-ConfigValue (Read-Config $legacyCfg).Text 'r_fov'),$culture)
            }
        } catch { }
    }
}
if ([double]::IsNaN($baseFov) -or [double]::IsInfinity($baseFov) -or $baseFov -le 0) { throw 'The original focal-length value is invalid.' }
$newFov=$baseFov*(4.0/3.0)/([double]$Width/$Height)
$newExe=[OldGamesFixesV2.Engine]::Build($layout,$Width,$Height,($InterfaceMode -eq 'Original'))
$newText=Set-ConfigValue $configData.Text 'r_xres' ($Width.ToString($culture))
$newText=Set-ConfigValue $newText 'r_yres' ($Height.ToString($culture))
$newText=Set-ConfigValue $newText 'r_fov' ($newFov.ToString('G17',$culture))
$newConfig=$configData.Encoding.GetBytes($newText)
$report=[ordered]@{Version=$version; Executable=$Executable; Configuration=$Config; Resolution=($Width.ToString()+'x'+$Height); InterfaceMode=$InterfaceMode; Engine=$layout.EngineFamily; ImageBase=$layout.ImageBase; GameplayAspectOffset=$layout.GameplayAspect; ForwardXOffset=$layout.ForwardX; RootCallOffset=$layout.RootCall; OriginalFocalLength=$baseFov; FocalLength=$newFov; ExecutableSha256=(Get-ByteHash $newExe)}
if ($Preview) { $report | ConvertTo-Json; return }

$stateExisted=Test-Path -LiteralPath $statePath
$oldStateBytes=$null
if ($stateExisted) { $oldStateBytes=[IO.File]::ReadAllBytes($statePath) }
if (!$state) {
    New-Item -ItemType Directory -Path $backupDir | Out-Null
    [IO.File]::WriteAllBytes($backupExe,$currentExe); [IO.File]::WriteAllBytes($backupCfg,$configData.Bytes)
}
$newState=[pscustomobject][ordered]@{Schema='Emergency4ResolutionFix/2'; ExecutableName=$Executable; ConfigName=$Config; BackupExeSha256=(Get-FileSha $backupExe); BackupCfgSha256=(Get-FileSha $backupCfg); BaselineSha256=(Get-ByteHash $layout.Baseline); OriginalFocalLength=$baseFov; InstalledExeSha256=(Get-ByteHash $newExe); Version=$version; Width=$Width; Height=$Height; InterfaceMode=$InterfaceMode}
# Persist a valid pre-write marker so ordinary failures leave usable backups.
if (!$stateExisted) { $newState.InstalledExeSha256=$currentHash; Write-Atomic $statePath ([Text.Encoding]::UTF8.GetBytes(($newState | ConvertTo-Json))) }
Write-Atomic $baselinePath $layout.Baseline
$newState.InstalledExeSha256=Get-ByteHash $newExe
try {
    Write-Atomic $exe $newExe; Write-Atomic $cfg $newConfig
    if ((Get-FileSha $exe) -ne (Get-ByteHash $newExe) -or (Get-FileSha $cfg) -ne (Get-ByteHash $newConfig)) { throw 'Written-file verification failed.' }
    Write-Atomic $statePath ([Text.Encoding]::UTF8.GetBytes(($newState | ConvertTo-Json)))
} catch {
    Write-Atomic $exe $currentExe; Write-Atomic $cfg $configData.Bytes
    if ($oldStateBytes) { Write-Atomic $statePath $oldStateBytes }
    throw
}
Write-Output "Fix applied: $Width x $Height; menu/HUD: $InterfaceMode. Start the game normally."
Write-Output "Detected executable: $Executable; configuration: $Config"
Write-Output "Backup: $backupDir"
