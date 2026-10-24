[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$system=[IO.Path]::GetFullPath((Join-Path $GameDirectory 'system'))
$exe=Join-Path $system 'sas.exe'
if (!(Test-Path -LiteralPath $exe) -or (Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne 'f7787e7c1aa58416bc3222259cbeac02f7431794f1c3678c28eba14404a90c66') { throw 'Unsupported game executable. No files changed.' }
if (Get-Process -Name sas -ErrorAction SilentlyContinue) { throw 'Close The Regiment before updating.' }
$plugin=Join-Path $system 'scripts/TheRegiment.FOVFix.asi'
$ini=Join-Path $system 'scripts/TheRegiment.FOVFix.ini'
if (!(Test-Path -LiteralPath $plugin) -or !(Test-Path -LiteralPath $ini)) { throw 'Existing ASI and INI required; use Install.ps1 for a fresh installation.' }
$version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead((Join-Path $root "release/TheRegiment.FOVFix-v$version.zip"))
$temporary=Join-Path $system 'scripts/TheRegiment.FOVFix.asi.update.tmp'
if (Test-Path -LiteralPath $temporary) { $archive.Dispose();throw 'A previous update temporary file exists; inspect it before retrying.' }
$oldHash=(Get-FileHash -LiteralPath $plugin).Hash.ToLowerInvariant()
$iniHash=(Get-FileHash -LiteralPath $ini).Hash
$backup=$null
$changed=$false
$manifestPath=Join-Path $system 'TheRegiment.FOVFix.install.json'
$manifestExisted=Test-Path -LiteralPath $manifestPath
try {
    $entry=$archive.GetEntry('system/scripts/TheRegiment.FOVFix.asi')
    if (!$entry) { throw 'Release archive is missing the ASI.' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$temporary,$false)
    $newHash=(Get-FileHash -LiteralPath $temporary).Hash.ToLowerInvariant()
    if ($oldHash -eq $newHash) { Write-Output 'Questa versione e gia installata.';return }
    $knownOld=@('27cea22995633604711e5b843bd03ea75a0ff158aec97d63fb0970dc1389fc6c','19291b2792629e13e6f0a44613ae60e2ff1c402d1419d72157f28c4d1f545387','31e308ea322409e6fed699ec6e0c3045d68aca691474ddf4e8bed651190edba8','ece386b298fdaf388d08674e9ab146e74cde82de4c5b7a852a9a8060a663bec2','afec88b7840460d49ccccfa324d5dfd33768d1992df5d27407258208e866fc35','354945d87d0a3e53094c3e101d6ab06ff15eca5a31870ee0524e1618ae8a565a')
    if ($oldHash -notin $knownOld) { throw 'The existing ASI is not a recognized 0.1.0, 0.2.0, 0.2.1, 0.2.2, 0.2.3 or 0.2.4 build. No overwrite performed.' }
    $backup=Join-Path $system ('TheRegiment.FOVFix-backups/'+[DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $backup | Out-Null
    # .bak extensions prevent the ASI loader from loading backup plugins.
    Copy-Item -LiteralPath $plugin -Destination (Join-Path $backup 'TheRegiment.FOVFix.asi.bak')
    Copy-Item -LiteralPath $ini -Destination (Join-Path $backup 'TheRegiment.FOVFix.ini.bak')
    if ($manifestExisted) { Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $backup 'install.json.bak') }
    $changed=$true
    Copy-Item -LiteralPath $temporary -Destination $plugin -Force
    if ((Get-FileHash -LiteralPath $plugin).Hash.ToLowerInvariant() -ne $newHash -or (Get-FileHash -LiteralPath $ini).Hash -ne $iniHash) { throw 'Update verification failed.' }
    if ($manifestExisted) {
        $manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        if ($manifest.id -ne 'TheRegiment.FOVFix') { throw 'Invalid installation manifest.' }
        foreach ($item in $manifest.created) { if ($item.name -eq 'scripts/TheRegiment.FOVFix.asi') { $item.sha256=$newHash } }
        $manifest.version=$version
        $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    }
    Write-Output "Aggiornato a $version. INI conservato: OSD automatico al cambio risoluzione attivo; nessuna nuova opzione richiesta. Backup: $backup"
} catch {
    if ($changed -and $backup) {
        Copy-Item -LiteralPath (Join-Path $backup 'TheRegiment.FOVFix.asi.bak') -Destination $plugin -Force
        if ($manifestExisted) { Copy-Item -LiteralPath (Join-Path $backup 'install.json.bak') -Destination $manifestPath -Force }
    }
    throw
} finally {
    $archive.Dispose()
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
}
