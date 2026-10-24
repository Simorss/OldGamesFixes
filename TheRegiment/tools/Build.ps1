[CmdletBinding()]
param([string]$Compiler = 'g++', [switch]$SkipGraphicsTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build'
$release = Join-Path $root 'release'
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
New-Item -ItemType Directory -Force -Path $build,$release | Out-Null
$compilerPath = (Get-Command $Compiler -ErrorAction Stop).Source
$machine = (& $compilerPath -dumpmachine | Out-String).Trim()
if ($machine -notmatch '^i[3-6]86-.*mingw') { throw "An x86 MinGW-w64 compiler is required; found: $machine" }
$common = @('-std=c++17','-O2','-Wall','-Wextra','-Werror','-static','-static-libgcc','-static-libstdc++')
$plugin = Join-Path $build 'TheRegiment.FOVFix.asi'
& $compilerPath @common -shared (Join-Path $root 'src/Plugin.cpp') (Join-Path $root 'src/Exports.def') -luser32 -ladvapi32 '-Wl,--dynamicbase,--nxcompat' -o $plugin
if ($LASTEXITCODE -ne 0) { throw 'Plugin compilation failed.' }
$runtime = Join-Path $build 'RuntimeTests.exe'
& $compilerPath @common (Join-Path $root 'tests/RuntimeTests.cpp') -luser32 -ladvapi32 -ld3d9 -o $runtime
if ($LASTEXITCODE -ne 0) { throw 'Runtime test compilation failed.' }
Push-Location $build
try {
    $runtimeArguments = @()
    if ($SkipGraphicsTests) { $runtimeArguments += '--skip-graphics' }
    $runtimeOutput = & $runtime @runtimeArguments 2>&1
    $runtimeExit = $LASTEXITCODE
    $runtimeOutput | ForEach-Object { Write-Output $_ }
    $testMatch = [regex]::Match(($runtimeOutput -join "`n"),'PASS: (\d+) checks')
    if ($runtimeExit -ne 0 -or !$testMatch.Success) { throw 'Runtime tests failed.' }
    $runtimeChecks = [int]$testMatch.Groups[1].Value
} finally { Pop-Location }
$loader = Join-Path $root 'third-party/Ultimate-ASI-Loader/dinput8.dll'
$loaderHash = 'ec2f4824eca58dd40f425756a4a7cec77b8e381f21d11d7c846ec4b339b617ab'
if ((Get-FileHash -LiteralPath $loader -Algorithm SHA256).Hash.ToLowerInvariant() -ne $loaderHash) { throw 'Official loader binary checksum mismatch.' }
$smoke = Join-Path $build 'smoke'
New-Item -ItemType Directory -Force -Path (Join-Path $smoke 'scripts') | Out-Null
$smokeExe = Join-Path $smoke 'LoaderSmoke.exe'
& $compilerPath @common (Join-Path $root 'tests/LoaderSmoke.cpp') -luser32 -ldinput8 -ldxguid -o $smokeExe
if ($LASTEXITCODE -ne 0) { throw 'Loader test compilation failed.' }
Copy-Item -LiteralPath $loader -Destination (Join-Path $smoke 'dinput8.dll') -Force
Copy-Item -LiteralPath $plugin -Destination (Join-Path $smoke 'scripts/TheRegiment.FOVFix.asi') -Force
$smokeLog = Join-Path $smoke 'scripts/TheRegiment.FOVFix.log'
$smokeIni = Join-Path $smoke 'scripts/TheRegiment.FOVFix.ini'
if (Test-Path -LiteralPath $smokeIni) { Remove-Item -LiteralPath $smokeIni }
if (Test-Path -LiteralPath $smokeLog) { Remove-Item -LiteralPath $smokeLog }
Push-Location $smoke
try { & $smokeExe; if ($LASTEXITCODE -ne 0) { throw 'Loader smoke process failed.' } }
finally { Pop-Location }
if (!(Test-Path -LiteralPath $smokeLog) -or (Get-Content -LiteralPath $smokeLog -Raw) -notmatch 'UNSUPPORTED executable. No camera or Direct3D hooks installed.') { throw 'Loader integration / unsupported-executable guard failed.' }
# Exercise the real ASI initialization in fresh processes. With the master off,
# initialization must return before even checking the host executable or migrating INI.
[IO.File]::WriteAllText($smokeIni,"[Main]`r`nEnabled=0`r`nConfigVersion=2`r`n[FOV]`r`nBaseFOV=83`r`n[Mouse]`r`nSensitivity=0.005`r`n",[Text.Encoding]::ASCII)
$disabledIniHash=(Get-FileHash -LiteralPath $smokeIni).Hash
Remove-Item -LiteralPath $smokeLog
Push-Location $smoke
try { & $smokeExe; if ($LASTEXITCODE -ne 0) { throw 'Disabled-loader smoke process failed.' } }
finally { Pop-Location }
$disabledLog=Get-Content -LiteralPath $smokeLog -Raw
if ($disabledLog -notmatch 'Disabled by \[Main\] Enabled=0. No hooks installed' -or $disabledLog -match 'UNSUPPORTED executable|Ready\.|Configuration migration' -or (Get-FileHash -LiteralPath $smokeIni).Hash -ne $disabledIniHash) { throw 'Global disabled startup failed or modified configuration.' }
[IO.File]::WriteAllText($smokeIni,"[Main]`r`nEnabled=1`r`n",[Text.Encoding]::ASCII)
Remove-Item -LiteralPath $smokeLog
Push-Location $smoke
try { & $smokeExe; if ($LASTEXITCODE -ne 0) { throw 'Reenabled-loader smoke process failed.' } }
finally { Pop-Location }
if ((Get-Content -LiteralPath $smokeLog -Raw) -notmatch 'UNSUPPORTED executable. No camera or Direct3D hooks installed.') { throw 'Reenabled startup did not reach the executable guard.' }
Remove-Item -LiteralPath $smokeIni
foreach ($binary in @($plugin,$loader)) {
    $data = [IO.File]::ReadAllBytes($binary)
    $pe = [BitConverter]::ToInt32($data,60)
    if ([BitConverter]::ToUInt16($data,$pe+4) -ne 0x14c) { throw "Not x86: $binary" }
}
$files = [ordered]@{}
$files[$plugin] = 'system/scripts/TheRegiment.FOVFix.asi'
$files[$loader] = 'system/dinput8.dll'
$files[(Join-Path $root 'config/TheRegiment.FOVFix.ini')] = 'system/scripts/TheRegiment.FOVFix.ini'
$files[(Join-Path $root 'third-party/Ultimate-ASI-Loader/LICENSE.txt')] = 'system/scripts/ThirteenAG-LICENSE.txt'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = Join-Path $release "TheRegiment.FOVFix-v$version.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip }
$archive = [IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Create)
try { foreach ($file in $files.Keys) { $null=[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$file,$files[$file],[IO.Compression.CompressionLevel]::Optimal) } }
finally { $archive.Dispose() }
$check = [IO.Compression.ZipFile]::OpenRead($zip)
try {
    if ($check.Entries.Count -ne $files.Count) { throw 'Unexpected release entries.' }
    foreach ($entry in $files.Values) { if (!$check.GetEntry($entry)) { throw "Missing: $entry" } }
    if ($check.Entries | Where-Object FullName -match '\.exe$') { throw 'The runtime release must not contain an EXE.' }
    if ($check.Entries | Where-Object FullName -match '\.md$') { throw 'Documentation belongs on GitHub, not in the runtime ZIP.' }
    if (@($check.Entries | Where-Object FullName -match '\.txt$').Count -ne 1) { throw 'The ZIP must contain exactly one third-party license TXT.' }
} finally { $check.Dispose() }
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
$checksumLines = @($hash+'  '+[IO.Path]::GetFileName($zip))
[IO.File]::WriteAllText((Join-Path $release 'SHA256SUMS.txt'),(($checksumLines -join "`r`n")+"`r`n"),[Text.Encoding]::ASCII)
$sourceHashes = [ordered]@{}
foreach ($name in @('src/Plugin.cpp','src/FovMath.h','src/InputMath.h','src/Settings.inl','src/Exports.def','tests/RuntimeTests.cpp','tests/TestExtensions.inl','tests/LoaderSmoke.cpp','tests/Test-Install.ps1','tests/Test-Update.ps1','config/TheRegiment.FOVFix.ini','tools/Build.ps1','tools/Install.ps1','tools/Update.ps1','tools/Uninstall.ps1')) {
    $sourceHashes[$name] = (Get-FileHash -LiteralPath (Join-Path $root $name) -Algorithm SHA256).Hash.ToLowerInvariant()
}
$report = [ordered]@{
    version=$version; built_at_utc=[DateTime]::UtcNow.ToString('o'); compiler=[IO.Path]::GetFileName($compilerPath);compiler_version=((& $compilerPath -dumpfullversion | Out-String).Trim()); target=$machine
    mouse_sensitivity_policy='GAME_MENU_ONLY';technical_tests='PASS';runtime_checks=$runtimeChecks;runtime_output=($runtimeOutput -join "`n")
    graphics_tests=$(if($SkipGraphicsTests){'SKIPPED_HEADLESS_CI'}else{'PASS'})
    loader_integration='PASS';unsupported_executable_guard='PASS';global_disable_startup='PASS: no hooks or INI changes; explicit reenable and missing-key default verified';release_contains_exe=$false
    in_game_test='PENDING_USER_MANUAL_TEST';game_launched=$false;game_installation_modified=$false
    supported_game_sha256='f7787e7c1aa58416bc3222259cbeac02f7431794f1c3678c28eba14404a90c66'
    previous_version_gameplay='0.1.0 confirmed working by user; 0.2.0 reported crashing on mission load'
    regression='Renderer interface must never be called as render-target dimensions; 0.2.0 source failed the corrected fixture as expected'
    plugin_sha256=(Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash.ToLowerInvariant()
    loader_sha256=$loaderHash;release_sha256=$hash;source_sha256=$sourceHashes
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root 'docs/validation.json') -Encoding UTF8
Write-Output "Release locale creata: $zip"
Write-Output 'Gameplay: verifica manuale separata. Lo script di build non pubblica online.'
