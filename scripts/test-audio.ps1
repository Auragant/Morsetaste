# SPDX-License-Identifier: GPL-3.0-only
# Silent tests of the production thread against a simulated audio transport.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $projectRoot '.tools/w64devkit/bin/g++.exe'
$test = Join-Path $projectRoot 'build/audio_output_tests.exe'
& $compiler '-std=c++17' '-O2' '-Wall' '-Wextra' '-Wpedantic' '-Werror' '-static' (Join-Path $projectRoot 'tests/audio_output_tests.cpp') '-o' $test '-lole32' '-luuid' '-lavrt'
if ($LASTEXITCODE -ne 0) { throw 'Audio test compilation failed' }
& $test
if ($LASTEXITCODE -ne 0) { throw "Audio thread test failed: exit $LASTEXITCODE" }