#pragma once
#ifndef Assert
#include <cstdio>
#define DEBUG_ASSERT(x, message)                                                                                                                                                   \
    if (!(x)) {                                                                                                                                                                    \
        printf("%s Line: %d: Assertion failed: %s\n", __FUNCTION__, __LINE__, message);                                                                                            \
        fflush(stdout);                                                                                                                                                            \
        abort();                                                                                                                                                                   \
    }

#define RELEASE_ASSERT(x, message)
#if FULL_DEBUG
#define Assert(x, message) DEBUG_ASSERT(x, message)
#else
#define Assert(x, message) RELEASE_ASSERT(x, message)(void) 0
#endif
#define Unreachable(message) Assert(false, message)
#endif
