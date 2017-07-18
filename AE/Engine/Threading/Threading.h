#pragma once

#include <thread>
#include <mutex>

namespace AE
{

using Mutex			= std::mutex;
using LockGuard		= std::lock_guard<Mutex>;

#define LOCK_GUARD_LINE_HELPER_COMBINE( str, line ) str##line
#define LOCK_GUARD_LINE_HELPER( str, line ) LOCK_GUARD_LINE_HELPER_COMBINE( str, line )
#define LOCK_GUARD( mutex ) LockGuard LOCK_GUARD_LINE_HELPER( LOCK_GUARD_macro_object_at_line_, __LINE__)( mutex )

}
