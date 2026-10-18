[CmdletBinding()]
param([string]$Compiler = 'cl', [string]$LoaderArchive)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$version = (Get-Content (Join-Path $PSScriptRoot 'asi/VERSION') -Raw).Trim()
$build = Join-Path $PSScriptRoot 'asi/.work/build'
$release = Join-Path $PSScriptRoot 'release'
New-Item -ItemType Directory -Force -Path $build,$release | Out-Null
$source = Join-Path $PSScriptRoot 'asi/src/Plugin.cpp'
if ((Get-Content $source -Raw) -notmatch ([regex]::Escape('Version="'+$version+'"'))) { throw 'Plugin and package versions differ.' }
if ($Compiler -eq 'cl' -and !(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Use an x86 Visual Studio Developer PowerShell, or pass -Compiler with the path to an x86 MinGW g++.' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual C++ x86 build tools were not found.' }
    $vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
    $environment = & $env:ComSpec /d /s /c "`"$vcvars`" >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the x86 compiler environment.' }
    foreach ($line in $environment) {
        if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process') }
    }
}
$compilerPath = (Get-Command $Compiler -ErrorAction Stop).Source
$msvc = [IO.Path]::GetFileNameWithoutExtension($compilerPath) -eq 'cl'
$plugin = Join-Path $build 'Emergency4.WidescreenFix.asi'
$definitions = Join-Path $PSScriptRoot 'asi/src/Exports.def'
if ($msvc) {
    & $compilerPath /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT /LD $source "/Fo$build/Plugin.obj" /link "/DEF:$definitions" "/OUT:$plugin" /MACHINE:X86 /DYNAMICBASE /NXCOMPAT user32.lib
} else {
    & $compilerPath -std=c++17 -O2 -Wall -Wextra -Werror -static -static-libgcc -static-libstdc++ -shared $source $definitions -luser32 '-Wl,--dynamicbase,--nxcompat' -o $plugin
}
if ($LASTEXITCODE -ne 0) { throw 'ASI compilation failed.' }
foreach ($name in @('EngineProbe','RuntimeTest','LoaderSmoke')) {
    $testSource = Join-Path $PSScriptRoot "asi/tests/$name.cpp"
    $testExe = Join-Path $build "$name.exe"
    if ($msvc) {
        $arguments = @('/nologo','/std:c++17','/EHsc','/W4','/WX','/O2','/MT',$testSource,"/Fo$build/$name.obj",'/link',"/OUT:$testExe",'/MACHINE:X86','/NXCOMPAT','user32.lib')
        if ($name -eq 'RuntimeTest') { $arguments += @('/BASE:0x20000000','/DYNAMICBASE:NO') }
        if ($name -eq 'LoaderSmoke') { $arguments += @('dinput8.lib','dxguid.lib') }
    } else {
        $arguments = @('-std=c++17','-O2','-Wall','-Wextra','-Werror','-static','-static-libgcc','-static-libstdc++',$testSource,'-luser32','-o',$testExe)
        if ($name -eq 'RuntimeTest') { $arguments += '-Wl,--image-base,0x20000000,--disable-dynamicbase,--nxcompat' }
        if ($name -eq 'LoaderSmoke') { $arguments += @('-ldinput8','-ldxguid') }
    }
    & $compilerPath @arguments
    if ($LASTEXITCODE -ne 0) { throw "Test tool compilation failed: $name" }
}
$loaderHash = '14b3a1ad018899571ac9aa01482977f3c6d49e6cba99f552d01c5acacd1315e1'
if (!$LoaderArchive) {
    $LoaderArchive = Join-Path $build 'Ultimate-ASI-Loader-v9.7.4.zip'
    if (!(Test-Path -LiteralPath $LoaderArchive)) {
        Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.4/Ultimate-ASI-Loader-NoPDB.zip' -OutFile $LoaderArchive
    }
}
if ((Get-FileHash -LiteralPath $LoaderArchive -Algorithm SHA256).Hash -ne $loaderHash) { throw 'Official loader archive checksum mismatch.' }
$loaderDir = Join-Path $build 'loader'
Expand-Archive -LiteralPath $LoaderArchive -DestinationPath $loaderDir -Force
$loader = Join-Path $loaderDir 'dinput8.dll'
if (!(Test-Path -LiteralPath $loader)) { throw 'Official loader is missing.' }
foreach ($binary in @($plugin,$loader)) {
    $data = [IO.File]::ReadAllBytes($binary)
    $pe = [BitConverter]::ToInt32($data,60)
    if ([BitConverter]::ToUInt16($data,$pe+4) -ne 0x14c) { throw "Not an x86 binary: $binary" }
}
$smoke = Join-Path $build 'smoke'
New-Item -ItemType Directory -Force -Path (Join-Path $smoke 'scripts') | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'LoaderSmoke.exe') -Destination (Join-Path $smoke 'LoaderSmoke.exe') -Force
Copy-Item -LiteralPath $loader -Destination (Join-Path $smoke 'dinput8.dll') -Force
Copy-Item -LiteralPath $plugin -Destination (Join-Path $smoke 'scripts/Emergency4.WidescreenFix.asi') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'asi/data/scripts/Emergency4.WidescreenFix.ini') -Destination (Join-Path $smoke 'scripts/Emergency4.WidescreenFix.ini') -Force
$smokeIni = Join-Path $smoke 'scripts/Emergency4.WidescreenFix.ini'
$smokeSettings = [IO.File]::ReadAllText($smokeIni).Replace('ResX = 0','ResX = 1920').Replace('ResY = 0','ResY = 1080')
[IO.File]::WriteAllText($smokeIni,$smokeSettings,[Text.Encoding]::ASCII)
$smokeLog = Join-Path $smoke 'scripts/Emergency4.WidescreenFix.log'
if (Test-Path -LiteralPath $smokeLog) { Remove-Item -LiteralPath $smokeLog }
& (Join-Path $smoke 'LoaderSmoke.exe')
if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $smokeLog)) { throw 'ASI loader integration failed.' }
if ((Get-Content -LiteralPath $smokeLog -Raw) -notmatch 'Initialization stopped: Engine recognition failed:') { throw 'Unexpected loader smoke-test result.' }
$reportPath = Join-Path $PSScriptRoot 'docs/validation-asi.json'
if (!(Test-Path -LiteralPath $reportPath)) { throw 'A private-fixture ASI report is required to package a release.' }
$report = Get-Content $reportPath -Raw | ConvertFrom-Json
if ($report.status -ne 'PASS' -or $report.version -ne $version) { throw 'The private-fixture ASI report is not passing for this version.' }
foreach ($property in $report.source_sha256.PSObject.Properties) {
    if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $property.Name) -Algorithm SHA256).Hash -ne $property.Value) { throw "Validation report is stale for $($property.Name)." }
}
$files = [ordered]@{
    $plugin = 'scripts/Emergency4.WidescreenFix.asi'
    $loader = 'dinput8.dll'
    (Join-Path $PSScriptRoot 'asi/data/scripts/Emergency4.WidescreenFix.ini') = 'scripts/Emergency4.WidescreenFix.ini'
    (Join-Path $PSScriptRoot 'asi/data/tools/Restore-LegacyFix.cmd') = 'tools/Restore-LegacyFix.cmd'
    (Join-Path $PSScriptRoot 'src/Emergency4-Fix.ps1') = 'tools/legacy/Emergency4-Fix.ps1'
    (Join-Path $PSScriptRoot 'src/EnginePatch.cs') = 'tools/legacy/EnginePatch.cs'
    (Join-Path $PSScriptRoot 'README.md') = 'README.md'
    (Join-Path $PSScriptRoot 'CHANGELOG.md') = 'CHANGELOG.md'
    (Join-Path $PSScriptRoot 'docs/RELEASE-NOTES.md') = 'docs/RELEASE-NOTES.md'
    (Join-Path $PSScriptRoot 'docs/TECHNICAL-ASI.md') = 'docs/TECHNICAL-ASI.md'
    $reportPath = 'docs/validation-asi.json'
    (Join-Path $PSScriptRoot 'asi/third-party/Ultimate-ASI-Loader-LICENSE.txt') = 'licenses/Ultimate-ASI-Loader-LICENSE.txt'
}
$manifest = [ordered]@{Version=$version; Architecture='x86'; CompilerFamily=$(if ($msvc) {'Visual C++'} else {'GCC/MinGW'}); LoaderVersion='9.7.4'; LoaderArchiveSha256=$loaderHash; Files=@()}
foreach ($file in $files.Keys) { $manifest.Files += [ordered]@{Path=$files[$file]; Sha256=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()} }
$manifestPath = Join-Path $build 'manifest.json'
[IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
$files[$manifestPath]='manifest.json'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = Join-Path $release "Emergency4.WidescreenFix-v$version.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip }
$archive = [IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Create)
try { foreach ($file in $files.Keys) { $null=[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$file,$files[$file],[IO.Compression.CompressionLevel]::Optimal) } }
finally { $archive.Dispose() }
$hash=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $release 'SHA256SUMS.txt'),($hash+'  '+[IO.Path]::GetFileName($zip)+[Environment]::NewLine),[Text.Encoding]::ASCII)
Write-Output "Release created: $zip"
