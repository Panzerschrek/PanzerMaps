#pragma once
#include <cassert>

// Simple assert wrapper.
// If you wish disable asserts, or do something else redefine this macro.
#ifdef PM_DEBUG
#define PM_ASSERT(x) \
	{ assert(x); }
#else
#define PM_ASSERT(x) {}
#endif

#define PM_UNUSED(x) (void)x
