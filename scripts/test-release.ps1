# SPDX-License-Identifier: GPL-3.0-only
# Test the shipped binary, not just a freshly compiled replacement.
param([ValidatePattern('^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$')][string]$Repository = 'Auragant/Morsetaste')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$releaseRoot = Join-Path $projectRoot ('build/release-check/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $releaseRoot | Out-Null
$headers = @{ Accept = 'application/vnd.github+json' }
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repository/releases/latest" -Headers $headers
$assets = @($release.assets | Where-Object { $_.name -match '^MorseBridge-\d+\.\d+\.\d+-win64\.zip$' })
if ($assets.Count -ne 1) { throw 'Expected exactly one published MorseBridge Windows package.' }
$asset = $assets[0]
if ($asset.digest -notmatch '^sha256:([a-f0-9]{64})$') { throw 'Release package SHA-256 is missing.' }
$expected = $Matches[1]
$archive = Join-Path $releaseRoot $asset.name
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $archive
$actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
if ($actual -ne $expected) { throw 'Release package SHA-256 mismatch.' }
Expand-Archive -LiteralPath $archive -DestinationPath $releaseRoot
$exe = Join-Path $releaseRoot 'MorseBridge/dist/MorseBridge.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw 'Published executable missing in release package.' }
$exeHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $exe).Hash.ToLowerInvariant()
$description = "Published release: $($release.tag_name); EXE SHA-256: $exeHash"
Write-Host $description
if ($env:GITHUB_STEP_SUMMARY) { Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Value $description }
& (Join-Path $PSScriptRoot 'test-gui.ps1') -Executable $exe -ImageDirectory (Join-Path $projectRoot 'build/gui-release')
