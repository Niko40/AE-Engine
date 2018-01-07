#pragma once

#include <inttypes.h>

#include "BUILD_OPTIONS.h"
#include "Threading/Threading.h"

#ifdef _WIN32

#define VK_USE_PLATFORM_WIN32_KHR 1

#undef APIENTRY
#include <windows.h>

#if BUILD_TODO_SHOW_WHEN_COMPILING
#define TODO( msg )			__pragma( message( " - TODO - " #msg ) )
#endif

#if _DEBUG
#define AE_DEBUG			1
#endif

#undef min
#undef max

#else

#error Please add platform support

#endif

#if !defined( TODO )
#define TODO( msg )			// Don't report todo's when compiling
#endif

#if !defined( AE_DEBUG )
// If debug isn't being used, define it 0
#define AE_DEBUG			0
#endif
