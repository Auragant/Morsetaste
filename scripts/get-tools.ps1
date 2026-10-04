# SPDX-License-Identifier: GPL-3.0-only
param([switch]$WindowsOnly)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolRoot = Join-Path $projectRoot '.tools'
New-Item -ItemType Directory -Force -Path $toolRoot | Out-Null

function Get-ReleaseAsset($repo, $tag, $name, $expectedSha256) {
    $release = Invoke-RestMethod "https://api.github.com/repos/$repo/releases/tags/$tag"
    $asset = $release.assets | Where-Object name -EQ $name
    if (-not $asset) { throw "Release asset missing: $name" }
    $destination = Join-Path $toolRoot $name
    if (-not (Test-Path -LiteralPath $destination)) {
        Write-Host "Download: $name"
        Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $destination
    }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash.ToLowerInvariant()
    if ($hash -ne $expectedSha256) { throw "Pinned SHA256 mismatch: $name" }
    if ($asset.digest -and $asset.digest -ne "sha256:$hash") { throw "Release SHA256 mismatch: $name" }
    Write-Host "SHA256 $name : $hash"
    return $destination
}

if (-not (Test-Path -LiteralPath (Join-Path $toolRoot 'w64devkit/bin/g++.exe'))) {
    $archive = Get-ReleaseAsset 'skeeto/w64devkit' 'v2.10.0' 'w64devkit-x64-2.10.0.7z.exe' '18d0a4c71a166f8401ab6305781bec5882b40b5e06ba9807c61cb5f3b3c6325e'
    $sevenZip = 'C:\Program Files\7-Zip\7z.exe'
    if (Test-Path -LiteralPath $sevenZip) {
        & $sevenZip x $archive "-o$toolRoot" -y -bso0
        if ($LASTEXITCODE -ne 0) { throw 'Compiler extraction failed' }
    } else {
        $extract = Start-Process -FilePath $archive -ArgumentList @('-y', "-o$toolRoot") -WorkingDirectory $toolRoot -WindowStyle Hidden -PassThru -Wait
        if ($extract.ExitCode -ne 0) { throw 'Compiler extraction failed' }
    }
}
if (-not $WindowsOnly -and -not (Test-Path -LiteralPath (Join-Path $toolRoot 'arduino-cli/arduino-cli.exe'))) {
    $archive = Get-ReleaseAsset 'arduino/arduino-cli' 'v1.5.1' 'arduino-cli_1.5.1_Windows_64bit.zip' 'fabe42e0eb04d00e776a66178299ff95a46c623dbc260f997e58fd514853dd40'
    Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $toolRoot 'arduino-cli') -Force
}
Write-Host 'Portable Werkzeuge liegen in .tools; keine systemweite Installation.'
