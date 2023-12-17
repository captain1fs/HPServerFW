#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

#define ASSERT(x) \
    if(!(x)) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" \
            << windgent::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define ASSERT2(x, w) \
    if(!(x)) { \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\n" << w << "\nbacktrace:\n" \
            << windgent::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif