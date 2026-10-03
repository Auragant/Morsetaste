# SPDX-License-Identifier: GPL-3.0-only
# Compatibility build with the installed Microsoft compiler and Windows SDK.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ Build Tools fehlen.' }
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Microsoft C++ toolchain not found.' }
& (Join-Path $installation 'Common7/Tools/Launch-VsDevShell.ps1') -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'Microsoft compiler environment unavailable.' }
Write-Host "Microsoft toolchain: $installation"
Write-Host "Windows SDK: $env:WindowsSDKVersion"

$output = Join-Path $projectRoot 'build/msvc'
New-Item -ItemType Directory -Force -Path $output | Out-Null
Push-Location $output
try {
    & rc.exe /nologo "/I$projectRoot/windows" /fo app.res (Join-Path $projectRoot 'windows/app.rc')
    if ($LASTEXITCODE -ne 0) { throw 'Microsoft resource compilation failed.' }
    $flags = @('/nologo','/std:c++17','/EHsc','/utf-8','/permissive-','/W3','/O2','/MT',
        '/DUNICODE','/D_UNICODE','/D_WIN32_WINNT=0x0A00')
    $libraries = @('setupapi.lib','comctl32.lib','gdi32.lib','user32.lib','advapi32.lib',
        'wtsapi32.lib','shell32.lib','uuid.lib','gdiplus.lib','ole32.lib','avrt.lib')
    & cl.exe @flags (Join-Path $projectRoot 'windows/main.cpp') app.res '/Fe:MorseBridge.exe' /link /SUBSYSTEM:WINDOWS @libraries
    if ($LASTEXITCODE -ne 0) { throw 'Microsoft C++ / Windows SDK compilation failed.' }
    foreach ($name in @('core','tone','audio_core','audio_output','firmware')) {
        $extra = @()
        if ($name -eq 'firmware') { $extra = @("/I$projectRoot/tests/arduino_stub") }
        & cl.exe @flags @extra (Join-Path $projectRoot "tests/${name}_tests.cpp") "/Fe:${name}_tests.exe" /link @libraries
        if ($LASTEXITCODE -ne 0) { throw "Microsoft compilation failed: $name" }
        & (Join-Path $output "${name}_tests.exe")
        if ($LASTEXITCODE -ne 0) { throw "Microsoft test failed: $name" }
    }
} finally { Pop-Location }
& (Join-Path $PSScriptRoot 'test-gui.ps1') -Executable (Join-Path $output 'MorseBridge.exe') -ImageDirectory (Join-Path $output 'gui')
