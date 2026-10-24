[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$system = [IO.Path]::GetFullPath((Join-Path $GameDirectory 'system'))
$exe = Join-Path $system 'sas.exe'
$supported = 'f7787e7c1aa58416bc3222259cbeac02f7431794f1c3678c28eba14404a90c66'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'sas.exe not found in GameDirectory/system.' }
if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $supported) { throw 'Unsupported executable; no game files changed.' }
if (Get-Process -Name sas -ErrorAction SilentlyContinue) { throw 'Close The Regiment before installing.' }
$manifestPath = Join-Path $system 'TheRegiment.FOVFix.install.json'
if (Test-Path -LiteralPath $manifestPath) { throw 'An installation manifest already exists. Uninstall the previous copy first.' }
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
$archivePath = Join-Path $root "release/TheRegiment.FOVFix-v$version.zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($archivePath)
$created = [Collections.Generic.List[object]]::new()
try {
    $runtime = @('dinput8.dll','scripts/TheRegiment.FOVFix.asi','scripts/TheRegiment.FOVFix.ini','scripts/ThirteenAG-LICENSE.txt')
    foreach ($name in $runtime) {
        $entry=$archive.GetEntry("system/$name");if (!$entry) { throw "Missing runtime file: $name" }
        $destination=Join-Path $system $name
        if (Test-Path -LiteralPath $destination) {
            if ($name -ne 'dinput8.dll' -or (Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne 'ec2f4824eca58dd40f425756a4a7cec77b8e381f21d11d7c846ec4b339b617ab') { throw "Existing file: $destination. No file was overwritten." }
        }
    }
    foreach ($name in $runtime) {
        $destination=Join-Path $system $name
        if (Test-Path -LiteralPath $destination) { continue }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        $created.Add([ordered]@{name=$name;sha256=$null})
        [IO.Compression.ZipFileExtensions]::ExtractToFile($archive.GetEntry("system/$name"),$destination,$false)
        $created[$created.Count-1].sha256=(Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant()
    }
    [ordered]@{id='TheRegiment.FOVFix';version=$version;created=@($created.ToArray())} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
} catch {
    foreach ($item in $created) { $file=Join-Path $system $item.name;if (Test-Path -LiteralPath $file -PathType Leaf) { Remove-Item -LiteralPath $file } }
    throw
} finally { $archive.Dispose() }
Write-Output "Installato in $system. F7/F8: camera; F11/F12: viewmodel; sensibilita dal menu del gioco."
