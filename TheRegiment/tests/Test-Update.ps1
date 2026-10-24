[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GameDirectory,[ValidateSet('0.1.0-preview','0.2.0-preview','0.2.1-preview','0.2.2-preview','0.2.3-preview','0.2.4-preview')][string]$FromVersion='0.1.0-preview')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$fixture=Join-Path $root ('build/update-fixture-'+$FromVersion+'-'+[Guid]::NewGuid().ToString('N'))
$system=Join-Path $fixture 'system'
$privateExe=Join-Path $system 'sas.exe'
if (Test-Path -LiteralPath $fixture) { throw 'Update fixture already exists; inspect it before rerunning.' }
New-Item -ItemType Directory -Force -Path (Join-Path $system 'scripts') | Out-Null
Copy-Item -LiteralPath (Join-Path $GameDirectory 'system/sas.exe') -Destination $privateExe
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $old=[IO.Compression.ZipFile]::OpenRead((Join-Path $root "release/TheRegiment.FOVFix-v$FromVersion.zip"))
    try {
        foreach ($name in @('system/dinput8.dll','system/scripts/TheRegiment.FOVFix.asi','system/scripts/TheRegiment.FOVFix.ini')) {
            [IO.Compression.ZipFileExtensions]::ExtractToFile($old.GetEntry($name),(Join-Path $fixture $name),$false)
        }
    } finally { $old.Dispose() }
    $ini=Join-Path $system 'scripts/TheRegiment.FOVFix.ini'
    [IO.File]::WriteAllText($ini,[IO.File]::ReadAllText($ini).Replace('BaseFOV = 70.000','BaseFOV = 78.000'))
    $iniHash=(Get-FileHash -LiteralPath $ini).Hash
    $loaderHash=(Get-FileHash -LiteralPath (Join-Path $system 'dinput8.dll')).Hash
    $plugin=Join-Path $system 'scripts/TheRegiment.FOVFix.asi'
    $oldHash=(Get-FileHash -LiteralPath $plugin).Hash.ToLowerInvariant()
    $manifestPath=Join-Path $system 'TheRegiment.FOVFix.install.json'
    [ordered]@{id='TheRegiment.FOVFix';version=$FromVersion;created=@([ordered]@{name='scripts/TheRegiment.FOVFix.asi';sha256=$oldHash},[ordered]@{name='scripts/TheRegiment.FOVFix.ini';sha256=$iniHash.ToLowerInvariant()})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    & (Join-Path $root 'tools/Update.ps1') -GameDirectory $fixture
    $expected=(Get-Content -LiteralPath (Join-Path $root 'docs/validation.json') -Raw | ConvertFrom-Json).plugin_sha256
    if ((Get-FileHash -LiteralPath $plugin).Hash.ToLowerInvariant() -ne $expected) { throw 'Updated ASI hash mismatch.' }
    if ((Get-FileHash -LiteralPath $ini).Hash -ne $iniHash) { throw 'User INI was modified during update.' }
    if ((Get-FileHash -LiteralPath (Join-Path $system 'dinput8.dll')).Hash -ne $loaderHash) { throw 'Loader was changed.' }
    $backup=@(Get-ChildItem -LiteralPath (Join-Path $system 'TheRegiment.FOVFix-backups') -Directory)
    if ($backup.Count -ne 1 -or (Get-FileHash -LiteralPath (Join-Path $backup[0].FullName 'TheRegiment.FOVFix.asi.bak')).Hash.ToLowerInvariant() -ne $oldHash) { throw 'Original plugin backup missing.' }
    if (Get-ChildItem -LiteralPath (Join-Path $system 'TheRegiment.FOVFix-backups') -Filter '*.asi' -Recurse -File) { throw 'Backup could be loaded as a second ASI.' }
    $manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $currentVersion=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
    if ($manifest.version -ne $currentVersion -or ($manifest.created | Where-Object name -eq 'scripts/TheRegiment.FOVFix.asi').sha256 -ne $expected) { throw 'Uninstall manifest was not updated.' }
    & (Join-Path $root 'tools/Update.ps1') -GameDirectory $fixture
    & (Join-Path $root 'tools/Uninstall.ps1') -GameDirectory $fixture
    if (Test-Path -LiteralPath $plugin) { throw 'Updated plugin was not uninstalled.' }
    if (!(Test-Path -LiteralPath (Join-Path $system 'dinput8.dll'))) { throw 'Preexisting loader was removed.' }
    Write-Output "PASS: update from $FromVersion, unchanged user INI and loader, safe backups, manifest upgrade, repeated update, uninstall."
} finally {
    $absolute=[IO.Path]::GetFullPath($privateExe)
    $allowed=[IO.Path]::GetFullPath((Join-Path $root 'build'))+[IO.Path]::DirectorySeparatorChar
    if (!$absolute.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($absolute) -ne 'sas.exe') { throw 'Unsafe fixture cleanup path.' }
    if (Test-Path -LiteralPath $privateExe) { Remove-Item -LiteralPath $privateExe -Force }
}
