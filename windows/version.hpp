// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Single source of truth for the application, resources and release scripts.
#define MB_VERSION_MAJOR 1
#define MB_VERSION_MINOR 2
#define MB_VERSION_PATCH 2
#define MB_STRINGIFY_IMPL(value) #value
#define MB_STRINGIFY(value) MB_STRINGIFY_IMPL(value)
#define MB_WIDEN_IMPL(value) L##value
#define MB_WIDEN(value) MB_WIDEN_IMPL(value)
#define MB_VERSION_STRING MB_STRINGIFY(MB_VERSION_MAJOR) "." MB_STRINGIFY(MB_VERSION_MINOR) "." MB_STRINGIFY(MB_VERSION_PATCH)
#define MB_VERSION_WSTRING MB_WIDEN(MB_STRINGIFY(MB_VERSION_MAJOR)) L"." MB_WIDEN(MB_STRINGIFY(MB_VERSION_MINOR)) L"." MB_WIDEN(MB_STRINGIFY(MB_VERSION_PATCH))
#define MB_VERSION_RC MB_VERSION_MAJOR,MB_VERSION_MINOR,MB_VERSION_PATCH,0

// Build metadata is generated once by build.ps1; analysis needs no generated file.
#ifdef MB_GENERATED_BUILD_INFO
#include "../build/build_info.hpp"
#else
#define MB_BUILD_UTC_STRING "unbekannt"
#define MB_BUILD_UTC_WSTRING L"unbekannt"
#endif
