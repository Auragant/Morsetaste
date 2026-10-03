# SPDX-License-Identifier: GPL-3.0-only
param([ValidatePattern('^\d+\.\d+\.\d+(?:p)?$')][string]$Version = '1.2.1')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
# A fresh staging directory prevents old files leaking into a later package.
$runName = $Version + '-' + [guid]::NewGuid().ToString('N')
$stage = Join-Path $projectRoot "build/package/$runName/MorseBridge"
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
$core = Join-Path $projectRoot '.tools/arduino-data/packages/arduino/hardware/avr/1.8.8'
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
$preview = Join-Path $projectRoot 'build/gui/gui-demo.png'
if (Test-Path -LiteralPath $preview) { Copy-Item -LiteralPath $preview -Destination (Join-Path $stage 'GUI-Vorschau.png') -Force }
$archive = Join-Path $projectRoot "dist/MorseBridge-$Version-win64.zip"
Compress-Archive -LiteralPath $stage -DestinationPath $archive -Force
Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $projectRoot 'dist/MorseBridge.exe'),$archive | Format-List
Get-Item -LiteralPath $archive | Select-Object FullName,Length
