# SPDX-License-Identifier: GPL-3.0-only
function Read-MorseBridgeVersion {
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)
    $header = [IO.File]::ReadAllText((Join-Path $ProjectRoot 'windows/version.hpp'))
    $parts = @{}
    foreach ($part in @('MAJOR', 'MINOR', 'PATCH')) {
        $match = [regex]::Match($header, "(?m)^#define MB_VERSION_$part ([0-9]+)\s*$")
        if (-not $match.Success) { throw "Missing numeric MB_VERSION_$part in windows/version.hpp" }
        $parts[$part] = [int]$match.Groups[1].Value
        if ($parts[$part] -gt 65535) { throw "MB_VERSION_$part exceeds Windows version range" }
    }
    $version = "$($parts.MAJOR).$($parts.MINOR).$($parts.PATCH)"
    [pscustomobject]@{
        Version = $version
        WindowsVersion = "$version.0"
        Major = $parts.MAJOR
        Minor = $parts.MINOR
        Patch = $parts.PATCH
    }
}
