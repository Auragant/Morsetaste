# SPDX-License-Identifier: GPL-3.0-only
param([ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version = '', [string]$ToolsRoot = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'version.ps1')
$projectVersion = Read-MorseBridgeVersion -ProjectRoot $projectRoot
if ($Version -and $Version -ne $projectVersion.Version) { throw 'Package version does not match windows/version.hpp' }
$Version = $projectVersion.Version
if (-not $ToolsRoot) { $ToolsRoot = Join-Path $projectRoot '.tools' }
# Required notices must contain text before a complete download is created.
foreach ($relative in @('LICENSE','licenses/GCC-exception-3.1.txt',
    'licenses/LGPL-2.1.txt','licenses/avr-libc-2.0.0.txt','licenses/MinGW-w64-runtime.txt')) {
    $notice = Join-Path $projectRoot $relative
    if (-not (Test-Path -LiteralPath $notice -PathType Leaf) -or
        [IO.File]::ReadAllText($notice).Trim().Length -lt 128) {
        throw "Required license notice missing or incomplete: $relative"
    }
}
$exe = Join-Path $projectRoot 'dist/MorseBridge.exe'
$exeVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
if ($exeVersion.FileVersion -ne $Version -or $exeVersion.ProductVersion -ne $Version) {
    throw 'EXE version does not match windows/version.hpp. Build and test the matching EXE before packaging.'
}
# A fresh staging directory prevents old files leaking into a later package.
$runName = $Version + '-' + [guid]::NewGuid().ToString('N')
$runRoot = Join-Path $projectRoot "build/package/$runName"
$stage = Join-Path $runRoot 'MorseBridge'
$firmwareOutput = Join-Path $stage 'dist/firmware'
New-Item -ItemType Directory -Force -Path $stage,$firmwareOutput | Out-Null
foreach ($file in @('README.md','KURZANLEITUNG.txt','TESTING.md','CHANGELOG.md',
    'LICENSE','DISCLAIMER.md','SECURITY.md','THIRD_PARTY_NOTICES.md','.gitignore','.gitattributes')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $stage -Force
}
foreach ($directory in @('firmware','windows','tests','scripts','docs','licenses','.github')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $directory) -Destination $stage -Recurse -Force
}
# Static Arduino core code in the HEX needs corresponding sources and notices.
# Keep the original third-party source headers unchanged; do not bundle drivers,
# tool executables, unrelated libraries or a bootloader binary.
$core = Join-Path $ToolsRoot 'arduino-data/packages/arduino/hardware/avr/1.8.8'
if (-not (Test-Path -LiteralPath (Join-Path $core 'cores/arduino/Arduino.h'))) {
    throw 'Arduino AVR Core 1.8.8 missing. Run scripts/build-firmware.ps1 -InstallCore first.'
}
$coreStage = Join-Path $stage 'third_party/arduino-avr-1.8.8'
New-Item -ItemType Directory -Force -Path $coreStage | Out-Null
foreach ($directory in @('cores','variants')) {
    Copy-Item -LiteralPath (Join-Path $core $directory) -Destination $coreStage -Recurse -Force
}
foreach ($file in @('boards.txt','platform.txt','programmers.txt','README.md')) {
    Copy-Item -LiteralPath (Join-Path $core $file) -Destination $coreStage -Force
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'licenses/LGPL-2.1.txt') -Destination (Join-Path $coreStage 'COPYING.LESSER') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'dist/MorseBridge.exe') -Destination (Join-Path $stage 'dist') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'dist/firmware/JunkerSpace.ino.hex') -Destination $firmwareOutput -Force
$binaryChecksums = foreach ($relative in @('dist/MorseBridge.exe','dist/firmware/JunkerSpace.ino.hex')) {
    $digest = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $stage $relative)).Hash.ToLowerInvariant()
    "$digest  $relative"
}
[IO.File]::WriteAllLines((Join-Path $stage 'SHA256SUMS.txt'), [string[]]$binaryChecksums, [Text.UTF8Encoding]::new($false))
$archive = Join-Path $projectRoot "dist/MorseBridge-$Version-win64.zip"
Compress-Archive -LiteralPath $stage -DestinationPath $archive -Force
$downloadChecksums = foreach ($relative in @("MorseBridge-$Version-win64.zip",'MorseBridge.exe','firmware/JunkerSpace.ino.hex')) {
    $file = Join-Path (Join-Path $projectRoot 'dist') $relative
    $digest = (Get-FileHash -Algorithm SHA256 -LiteralPath $file).Hash.ToLowerInvariant()
    "$digest  $relative"
}
[IO.File]::WriteAllLines((Join-Path $projectRoot 'dist/SHA256SUMS.txt'), [string[]]$downloadChecksums, [Text.UTF8Encoding]::new($false))
# Remove only this run's staging copy, after the archive was created successfully.
$packageRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build/package'))
$resolvedRun = (Resolve-Path -LiteralPath $runRoot).ProviderPath
if (-not $resolvedRun.StartsWith($packageRoot + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) { throw 'Staging path outside package directory.' }
Remove-Item -LiteralPath $resolvedRun -Recurse -Force
Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $projectRoot 'dist/MorseBridge.exe'),$archive | Format-List
Get-Item -LiteralPath $archive | Select-Object FullName,Length
