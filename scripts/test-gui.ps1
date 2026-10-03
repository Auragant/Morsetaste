# SPDX-License-Identifier: GPL-3.0-only
param([string]$Executable = '', [string]$ImageDirectory = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $Executable) { $Executable = Join-Path $projectRoot 'dist/MorseBridge.exe' }
if (-not $ImageDirectory) { $ImageDirectory = Join-Path $projectRoot 'build/gui' }
$exe = (Resolve-Path -LiteralPath $Executable).ProviderPath
$images = [IO.Path]::GetFullPath($ImageDirectory)
New-Item -ItemType Directory -Force -Path $images | Out-Null
$process = Start-Process -FilePath $exe -ArgumentList @('--render-test', ('"' + $images + '"')) -WindowStyle Hidden -PassThru
if (-not $process.WaitForExit(60000)) {
    $process.Kill()
    throw 'GUI / thread test timed out after 60 seconds.'
}
$process.Refresh()
if ($process.ExitCode -ne 0) { throw "GUI / thread test failed: exit $($process.ExitCode)" }
Write-Host 'PASS: GUI, COM selection, demo worker, pause/resume, close and thread shutdown.'
Get-ChildItem -LiteralPath $images | Select-Object Name,Length
