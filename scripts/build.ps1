# SPDX-License-Identifier: GPL-3.0-only
param([string]$Compiler = '', [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $Compiler) { $Compiler = Join-Path $projectRoot '.tools/w64devkit/bin/g++.exe' }
if (-not (Test-Path -LiteralPath $Compiler)) {
    $found = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($found) { $Compiler = $found.Source } else { throw 'Compiler fehlt. Zuerst scripts/get-tools.ps1 ausfuehren.' }
}
$compilerDirectory = Split-Path -Parent $Compiler
$windres = Join-Path $compilerDirectory 'windres.exe'
$output = Join-Path $projectRoot 'dist'
$build = Join-Path $projectRoot 'build'
New-Item -ItemType Directory -Force -Path $output,$build | Out-Null
Push-Location $projectRoot
try {
    & $windres '-Iwindows' 'windows/app.rc' '-O' 'coff' '-o' 'build/app.res'
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed' }
    $flags = @('-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-DUNICODE','-D_UNICODE','-D_WIN32_WINNT=0x0A00','-static','-static-libgcc','-static-libstdc++')
    & $Compiler @flags '-mwindows' '-municode' 'windows/main.cpp' 'build/app.res' '-o' 'dist/MorseBridge.exe' '-lsetupapi' '-lcomctl32' '-lgdi32' '-luser32' '-ladvapi32' '-lwtsapi32' '-lshell32' '-luuid' '-lgdiplus'
    if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed' }
    if (-not $SkipTests) {
        & $Compiler @flags 'tests/core_tests.cpp' '-o' 'build/core_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
        & './build/core_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Core tests failed' }
        & $Compiler @flags '-Itests/arduino_stub' 'tests/firmware_tests.cpp' '-o' 'build/firmware_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Firmware test compilation failed' }
        & './build/firmware_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Firmware tests failed' }
    }
    Get-Item -LiteralPath (Join-Path $output 'MorseBridge.exe') | Select-Object FullName,Length,LastWriteTime
} finally { Pop-Location }
