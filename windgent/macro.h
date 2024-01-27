#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
//告诉编译器出现条件 x 的概率大/小
#define WINDGENT_LIKELY(x)        __builtin_expect(!!(x), 1)
#define WINDGENT_UNLIKELY(x)      __builtin_expect(!!(x), 0)
#else
#define WINDGENT_LIKELY(x)      (x)
#define WINDGENT_UNLIKELY(x)    (x)
#endif

#define ASSERT(x) \
    if(WINDGENT_UNLIKELY(!(x))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" \
            << windgent::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define ASSERT2(x, w) \
    if(WINDGENT_UNLIKELY(!(x))) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\n" << w << "\nbacktrace:\n" \
            << windgent::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif