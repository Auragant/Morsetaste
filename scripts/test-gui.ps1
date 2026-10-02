# SPDX-License-Identifier: GPL-3.0-only
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $projectRoot 'dist/MorseBridge.exe'
$images = Join-Path $projectRoot 'build/gui'
New-Item -ItemType Directory -Force -Path $images | Out-Null
$process = Start-Process -FilePath $exe -ArgumentList @('--render-test', ('"' + $images + '"')) -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "GUI / thread test failed: exit $($process.ExitCode)" }
Write-Host 'PASS: GUI, COM selection, demo worker, pause/resume, close and thread shutdown.'
Get-ChildItem -LiteralPath $images | Select-Object Name,Length
