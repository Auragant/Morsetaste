# SPDX-License-Identifier: GPL-3.0-only
param([switch]$InstallCore, [string]$ToolsRoot = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $ToolsRoot) { $ToolsRoot = Join-Path $projectRoot '.tools' }
$cli = Join-Path $ToolsRoot 'arduino-cli/arduino-cli.exe'
if (-not (Test-Path -LiteralPath $cli)) { throw 'Zuerst scripts/get-tools.ps1 ausfuehren.' }
$names = @('ARDUINO_DIRECTORIES_DATA','ARDUINO_DIRECTORIES_DOWNLOADS','ARDUINO_DIRECTORIES_USER')
$saved = @{}
foreach ($name in $names) { $saved[$name] = [Environment]::GetEnvironmentVariable($name,'Process') }
try {
    $env:ARDUINO_DIRECTORIES_DATA = Join-Path $ToolsRoot 'arduino-data'
    $env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $ToolsRoot 'arduino-downloads'
    $env:ARDUINO_DIRECTORIES_USER = Join-Path $ToolsRoot 'arduino-user'
    if ($InstallCore) {
        & $cli core update-index
        if ($LASTEXITCODE -ne 0) { throw 'Arduino index download failed' }
        & $cli core install arduino:avr@1.8.8
        if ($LASTEXITCODE -ne 0) { throw 'Arduino AVR core installation failed' }
    }
    & $cli compile --fqbn 'arduino:avr:nano:cpu=atmega328' --warnings all --output-dir (Join-Path $projectRoot 'dist/firmware') --build-path (Join-Path $projectRoot 'build/arduino') (Join-Path $projectRoot 'firmware/JunkerSpace')
    if ($LASTEXITCODE -ne 0) { throw 'Firmware compilation failed' }
    & $cli core list
} finally {
    foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name,$saved[$name],'Process') }
}
