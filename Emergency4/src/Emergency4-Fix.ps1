# OldGamesFixes / Emergency4 - 1.0.0-beta.1
# Requires Windows PowerShell 5.1 or PowerShell 7. No downloads or elevation.
[CmdletBinding()]
param(
    [string]$GamePath,
    [int]$Width = 0,
    [int]$Height = 0,
    [ValidateSet('Original','Stretched')][string]$InterfaceMode,
    [switch]$Restore,
    [switch]$Preview,
    [switch]$NonInteractive
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$version = '1.0.0-beta.1'
$culture = [Globalization.CultureInfo]::InvariantCulture
$canonicalHash = '17924F0E847FB0173DDBD5FDB5D5CBBF68F75CF1DB6FDCADB155F964A350FDC9'
$legacyHashes = @('E3C7A79570D405E3E7E7ADEDB840E382405ED12CF11049841C7529C24ED0A8F0', '43AD206E3BFA3EF8FE972D754A36C0AD8EAE82EBCE0B7E8EBFA478AACDA8632E')

function Get-ByteHash([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-','') }
    finally { $sha.Dispose() }
}
function Get-FileSha([string]$Path) { return Get-ByteHash ([IO.File]::ReadAllBytes($Path)) }
function Convert-Hex([string]$Hex) {
    if ($Hex.Length % 2) { throw 'Manifest: stringa esadecimale non valida.' }
    $result = New-Object byte[] ($Hex.Length / 2)
    for ($i=0; $i -lt $result.Length; $i++) { $result[$i] = [Convert]::ToByte($Hex.Substring($i*2,2),16) }
    return ,$result
}
function Float-Bytes([double]$Value) { return ,([BitConverter]::GetBytes([single]$Value)) }
function Make-Branch([byte]$Opcode, [int]$From, [int]$To) {
    return ,([byte[]](@($Opcode) + [BitConverter]::GetBytes([int]($To-$From-5))))
}
function Read-Config([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $encoding = [Text.Encoding]::GetEncoding(1252)
    if ($bytes.Length -ge 2 -and $bytes[0] -eq 255 -and $bytes[1] -eq 254) { $encoding = [Text.Encoding]::Unicode }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 254 -and $bytes[1] -eq 255) { $encoding = [Text.Encoding]::BigEndianUnicode }
    elseif ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) { $encoding = [Text.Encoding]::UTF8 }
    # Decoding and re-encoding the BOM as a character preserves it exactly.
    return @{ Bytes=$bytes; Encoding=$encoding; Text=$encoding.GetString($bytes) }
}
function Get-ConfigValue([string]$Text,[string]$Key) {
    $pattern = '<var\s+name="'+[regex]::Escape($Key)+'"\s+value="([^"]*)"\s*/>'
    $matches = [regex]::Matches($Text,$pattern)
    if ($matches.Count -ne 1) { throw "Parametro $Key assente o duplicato nella configurazione." }
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
    } finally {
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force }
    }
}
function Normalize-Executable([byte[]]$Bytes,$Manifest) {
    $normal = [byte[]]$Bytes.Clone()
    foreach ($patch in $Manifest.patches) {
        $old = Convert-Hex $patch.original
        $offset = [int]$patch.offset
        if ($offset -lt 0 -or $offset+$old.Length -gt $normal.Length) { throw 'Manifest incompatibile con il file.' }
        [Array]::Copy($old,0,$normal,$offset,$old.Length)
    }
    if ((Get-ByteHash $normal) -ne $canonicalHash) {
        throw 'Eseguibile non supportato o modificato da altre patch. Nessuna modifica effettuata.'
    }
    return ,$normal
}
function Build-Executable([byte[]]$Original,$Manifest,[int]$W,[int]$H,[string]$Mode) {
    $result = [byte[]]$Original.Clone()
    $values = @{ GameplayAspect=(Float-Bytes ([double]$W/$H)) }
    if ($Mode -eq 'Original') {
        # Contain the original 1024x768 interface, also on displays narrower than 4:3.
        $scale = [Math]::Min($W/1024.0,$H/768.0)
        $canvasW = $W/$scale; $canvasH = $H/$scale
        $originX = ($canvasW-1024.0)/2; $originY = ($canvasH-768.0)/2
        foreach ($id in @('UiMouseWidth','UiWidth','UiClipWidth')) { $values[$id] = Float-Bytes $canvasW }
        foreach ($id in @('UiMouseHeight','UiHeight','UiClipHeight')) { $values[$id] = Float-Bytes $canvasH }
        $values.UiReciprocalWidth = Float-Bytes (1.0/$canvasW)
        $values.UiReciprocalY = Float-Bytes (1.0/$canvasH)
        $values.UiRootCall = Make-Branch 0xE8 0x934477 0x938401
        # Two 13-byte stubs in separate 15-byte alignment gaps. Stack shape is unchanged.
        $values.UiRootXStub = [byte[]](@(0xC7,0x44,0x24,0x04)+(Float-Bytes $originX)+(Make-Branch 0xE9 0x938409 0x938421))
        $values.UiRootYStub = [byte[]](@(0xC7,0x44,0x24,0x08)+(Float-Bytes $originY)+(Make-Branch 0xE9 0x938429 0x93A110))
        foreach ($patch in $Manifest.patches) {
            if ($patch.id -like 'UiYReference_*') { $values[$patch.id] = [BitConverter]::GetBytes([int]0x938441) }
        }
    }
    foreach ($patch in $Manifest.patches) {
        if ($values.ContainsKey($patch.id)) {
            $new = [byte[]]$values[$patch.id]
            if ($new.Length*2 -ne $patch.original.Length) { throw "Lunghezza patch errata: $($patch.id)." }
            [Array]::Copy($new,0,$result,[int]$patch.offset,$new.Length)
        }
    }
    return ,$result
}

if (!$NonInteractive -and !$Restore -and !$Preview -and $Width -eq 0 -and $Height -eq 0 -and !$InterfaceMode) {
    Write-Host "Emergency 4 Resolution Fix $version"
    Write-Host '1 - Applica / cambia risoluzione e interfaccia'
    Write-Host '2 - Ripristina la situazione precedente a questo script'
    Write-Host '0 - Esci'
    do { $action = Read-Host 'Scelta [1]'; if (!$action) { $action='1' } } until ($action -in @('0','1','2'))
    if ($action -eq '0') { return }
    if ($action -eq '2') { $Restore=$true }
}
if (!$GamePath) {
    if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Em4Deluxe.exe')) { $GamePath=$PSScriptRoot }
    elseif ($NonInteractive) { throw 'Specificare -GamePath.' }
    else { $GamePath=(Read-Host 'Cartella del gioco contenente Em4Deluxe.exe').Trim().Trim('"') }
}
$GamePath = (Resolve-Path -LiteralPath $GamePath).ProviderPath
$exe = Join-Path $GamePath 'Em4Deluxe.exe'
$cfg = Join-Path $GamePath 'em4deluxe.cfg'
if (!(Test-Path -LiteralPath $exe -PathType Leaf) -or !(Test-Path -LiteralPath $cfg -PathType Leaf)) {
    throw 'Servono Em4Deluxe.exe ed em4deluxe.cfg. Altre edizioni non sono ancora supportate.'
}
if (Get-Process -Name Em4Deluxe -ErrorAction SilentlyContinue) { throw 'Chiudere Emergency 4 prima di continuare.' }
$backupDir = Join-Path $GamePath 'OldGamesFixes-Emergency4-Backup'
$backupExe = Join-Path $backupDir 'Em4Deluxe.exe'
$backupCfg = Join-Path $backupDir 'em4deluxe.cfg'
$statePath = Join-Path $backupDir 'state.json'
$state = $null
if (Test-Path -LiteralPath $backupDir) {
    foreach ($path in @($backupExe,$backupCfg,$statePath)) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw 'Backup incompleto: non verra'' sovrascritto.' }
    }
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    if ($state.Schema -ne 'Emergency4ResolutionFix/1') { throw 'Formato backup sconosciuto.' }
    if ((Get-FileSha $backupExe) -ne $state.BackupExeSha256 -or (Get-FileSha $backupCfg) -ne $state.BackupCfgSha256) {
        throw 'Integrita'' del backup non valida: operazione interrotta.'
    }
}
$currentExe = [IO.File]::ReadAllBytes($exe)
$currentHash = Get-ByteHash $currentExe
$config = Read-Config $cfg
if ($Restore) {
    if (!$state) { throw 'Nessun backup di questo script trovato.' }
    if ($currentHash -notin @($state.InstalledExeSha256,$state.BackupExeSha256)) { throw 'Eseguibile modificato dopo il fix: ripristino interrotto.' }
    if ($Preview) { Write-Output "Ripristinerebbe il backup in $backupDir"; return }
    # Keep any settings changed since installation in a separate recovery file.
    $recoveryCfg = Join-Path $backupDir ('config-before-restore-'+[Guid]::NewGuid().ToString('N')+'.cfg')
    [IO.File]::WriteAllBytes($recoveryCfg,$config.Bytes)
    try {
        Write-Atomic $exe ([IO.File]::ReadAllBytes($backupExe))
        Write-Atomic $cfg ([IO.File]::ReadAllBytes($backupCfg))
    } catch {
        Write-Atomic $exe $currentExe; Write-Atomic $cfg $config.Bytes
        throw
    }
    Write-Output 'Ripristinati eseguibile e configurazione presenti prima del primo utilizzo di questo script.'
    return
}

$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'patches.json') -Raw | ConvertFrom-Json
if ($manifest.schema -ne 1 -or $manifest.originalSha256 -ne $canonicalHash) { throw 'Manifest non riconosciuto.' }
$knownHashes = @($canonicalHash)+$legacyHashes
if ($state) { $knownHashes += @($state.InstalledExeSha256,$state.BackupExeSha256) }
if ($currentHash -notin $knownHashes) { throw 'Eseguibile non riconosciuto. Nessuna modifica effettuata.' }
$original = Normalize-Executable $currentExe $manifest
$currentW = Get-ConfigValue $config.Text 'r_xres'
$currentH = Get-ConfigValue $config.Text 'r_yres'
if ($Width -eq 0 -or $Height -eq 0) {
    if ($NonInteractive) { throw 'Specificare -Width e -Height.' }
    do {
        $resolution = Read-Host "Risoluzione, es. 3440x1440 [$($currentW)x$($currentH)]"
        if (!$resolution) { $resolution=$currentW+'x'+$currentH }
        $match = [regex]::Match($resolution,'^\s*(\d{3,5})\s*[xX*]\s*(\d{3,5})\s*$')
        if ($match.Success) { $Width=[int]$match.Groups[1].Value; $Height=[int]$match.Groups[2].Value }
    } until ($match.Success -and $Width -ge 320 -and $Width -le 16384 -and $Height -ge 200 -and $Height -le 16384)
}
if ($Width -lt 320 -or $Width -gt 16384 -or $Height -lt 200 -or $Height -gt 16384) { throw 'Dimensioni ammesse: larghezza 320..16384, altezza 200..16384.' }
if (!$InterfaceMode) {
    if ($NonInteractive) { throw 'Specificare -InterfaceMode Original oppure Stretched.' }
    Write-Host '1 - Menu e HUD nelle proporzioni originali 4:3, centrati'
    Write-Host '2 - Menu e HUD estesi a schermo intero (possono apparire allargati)'
    do { $choice=Read-Host 'Interfaccia [1]'; if (!$choice) { $choice='1' } } until ($choice -in @('1','2'))
    if ($choice -eq '1') { $InterfaceMode='Original' } else { $InterfaceMode='Stretched' }
}
if ($state) { $baseFov = [double]$state.OriginalFocalLength }
elseif ($currentHash -in $legacyHashes) { $baseFov=350.0 }
else { $baseFov=[double]::Parse((Get-ConfigValue $config.Text 'r_fov'),$culture) }
if ([double]::IsNaN($baseFov) -or [double]::IsInfinity($baseFov) -or $baseFov -le 0) { throw 'Parametro focale di origine non valido.' }
$newFov = $baseFov*(4.0/3.0)/([double]$Width/$Height)
$newExe = Build-Executable $original $manifest $Width $Height $InterfaceMode
$newText = Set-ConfigValue $config.Text 'r_xres' ($Width.ToString($culture))
$newText = Set-ConfigValue $newText 'r_yres' ($Height.ToString($culture))
$newText = Set-ConfigValue $newText 'r_fov' ($newFov.ToString('G17',$culture))
$newConfig = $config.Encoding.GetBytes($newText)
$report = [ordered]@{ Version=$version; Resolution=($Width.ToString()+'x'+$Height); InterfaceMode=$InterfaceMode; OriginalFocalLength=$baseFov; FocalLength=$newFov; ExecutableSha256=(Get-ByteHash $newExe) }
if ($Preview) { $report | ConvertTo-Json; return }

if (!$state) {
    New-Item -ItemType Directory -Path $backupDir | Out-Null
    [IO.File]::WriteAllBytes($backupExe,$currentExe)
    [IO.File]::WriteAllBytes($backupCfg,$config.Bytes)
    $state = [pscustomobject][ordered]@{ Schema='Emergency4ResolutionFix/1'; BackupExeSha256=$currentHash; BackupCfgSha256=(Get-ByteHash $config.Bytes); OriginalFocalLength=$baseFov; InstalledExeSha256=$currentHash; Version=$version; Width=$Width; Height=$Height; InterfaceMode=$InterfaceMode }
    Write-Atomic $statePath ([Text.Encoding]::UTF8.GetBytes(($state | ConvertTo-Json)))
}
$oldStateBytes = [IO.File]::ReadAllBytes($statePath)
$state.Version=$version; $state.Width=$Width; $state.Height=$Height; $state.InterfaceMode=$InterfaceMode
$state.InstalledExeSha256=Get-ByteHash $newExe
try {
    Write-Atomic $exe $newExe
    Write-Atomic $cfg $newConfig
    if ((Get-FileSha $exe) -ne (Get-ByteHash $newExe) -or (Get-FileSha $cfg) -ne (Get-ByteHash $newConfig)) { throw 'Verifica dei file scritti fallita.' }
    Write-Atomic $statePath ([Text.Encoding]::UTF8.GetBytes(($state | ConvertTo-Json)))
} catch {
    Write-Atomic $exe $currentExe; Write-Atomic $cfg $config.Bytes; Write-Atomic $statePath $oldStateBytes
    throw
}
Write-Output "Fix applicato: $Width x $Height; menu/HUD: $InterfaceMode. Avvia il gioco normalmente."
Write-Output "Backup: $backupDir"
