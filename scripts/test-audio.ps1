# SPDX-License-Identifier: GPL-3.0-only
# Plays three short tones on the default Windows audio device; no serial/key input.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = Join-Path $projectRoot '.tools/w64devkit/bin/g++.exe'
$test = Join-Path $projectRoot 'build/audio_output_tests.exe'
& $compiler '-std=c++17' '-O2' '-Wall' '-Wextra' '-Wpedantic' '-Werror' '-static' (Join-Path $projectRoot 'tests/audio_output_tests.cpp') '-o' $test '-lwinmm'
if ($LASTEXITCODE -ne 0) { throw 'Audio test compilation failed' }
& $test
if ($LASTEXITCODE -ne 0) { throw "Audio device test failed: exit $LASTEXITCODE" }
