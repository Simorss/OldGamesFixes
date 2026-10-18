$ErrorActionPreference = 'Stop'
$version = '1.1.0-beta.1'
$releaseDir = Join-Path $PSScriptRoot 'release'
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
$zipPath = Join-Path $releaseDir "Emergency4-ResolutionFix-v$version.zip"
$files = [ordered]@{
    'src/Emergency4-Fix.ps1'='Emergency4-Fix.ps1'
    'src/Emergency4-Fix.cmd'='Emergency4-Fix.cmd'
    'src/EnginePatch.cs'='EnginePatch.cs'
    'README.md'='README.md'
    'CHANGELOG.md'='CHANGELOG.md'
    'docs/TECHNICAL.md'='docs/TECHNICAL.md'
    'docs/RELEASE-NOTES.md'='docs/RELEASE-NOTES.md'
    'docs/validation.json'='docs/validation.json'
}
foreach ($source in $files.Keys) {
    if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot $source) -PathType Leaf)) { throw "Missing file: $source" }
}
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath }
$archive = [IO.Compression.ZipFile]::Open($zipPath,[IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($source in $files.Keys) {
        $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,(Join-Path $PSScriptRoot $source),$files[$source],[IO.Compression.CompressionLevel]::Optimal)
    }
} finally { $archive.Dispose() }
$sha = [Security.Cryptography.SHA256]::Create()
try { $hash = [BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($zipPath))).Replace('-','') }
finally { $sha.Dispose() }
[IO.File]::WriteAllText((Join-Path $releaseDir 'SHA256SUMS.txt'),($hash+'  '+[IO.Path]::GetFileName($zipPath)+[Environment]::NewLine),[Text.Encoding]::ASCII)
Write-Output "Release created: $zipPath"
