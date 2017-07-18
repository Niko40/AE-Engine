#pragma once

#include <inttypes.h>

#include "BUILD_OPTIONS.h"
#include "Threading/Threading.h"

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1

#undef APIENTRY
#include <windows.h>

#define TODO( msg )			__pragma( message( " - TODO - " #msg ) )

#undef min
#undef max

#else

#error Please add platform support

#endif
