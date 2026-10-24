[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
if (Get-Process -Name sas -ErrorAction SilentlyContinue) { throw 'Close The Regiment before uninstalling.' }
$system = [IO.Path]::GetFullPath((Join-Path $GameDirectory 'system'))
$manifestPath = Join-Path $system 'TheRegiment.FOVFix.install.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.id -ne 'TheRegiment.FOVFix') { throw 'Invalid installation manifest.' }
$allowed = @('dinput8.dll','scripts/TheRegiment.FOVFix.asi','scripts/TheRegiment.FOVFix.ini','scripts/ThirteenAG-LICENSE.txt')
$removals = @()
foreach ($item in $manifest.created) {
    if ($item.name -cnotin $allowed) { throw 'Unexpected manifest path. No files removed.' }
    $path = [IO.Path]::GetFullPath((Join-Path $system $item.name))
    if (!$path.StartsWith($system.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Path outside game system directory.' }
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { continue }
    if ($item.name -ne 'scripts/TheRegiment.FOVFix.ini' -and (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $item.sha256) { throw "File changed after installation: $path. No files removed." }
    if ($item.name -eq 'dinput8.dll') {
        $other = @(Get-ChildItem -LiteralPath $system -Filter '*.asi' -File -ErrorAction SilentlyContinue)
        foreach ($dir in @('scripts','plugins','update')) { $other += @(Get-ChildItem -LiteralPath (Join-Path $system $dir) -Filter '*.asi' -File -Recurse -ErrorAction SilentlyContinue | Where-Object Name -ne 'TheRegiment.FOVFix.asi') }
        if ($other.Count) { Write-Output 'Loader conservato: sono presenti altri plugin ASI.';continue }
    }
    $removals += $path
}
foreach ($path in $removals) { Remove-Item -LiteralPath $path }
$log=Join-Path $system 'scripts/TheRegiment.FOVFix.log'
if (Test-Path -LiteralPath $log -PathType Leaf) { Remove-Item -LiteralPath $log }
Remove-Item -LiteralPath $manifestPath
Write-Output 'Fix disinstallato. File originali del gioco non modificati.'
