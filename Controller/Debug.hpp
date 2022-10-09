//
//  Debug.hpp
//  Controller
//
//  Created by FireWolf on 2/11/20.
//  Revised by FireWolf on 6/9/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef Debug_h
#define Debug_h

#include <stdarg.h>

#ifndef __cplusplus
    #define __PRETTY_FUNCTION__ __func__
#endif

#ifdef _MSC_VER
    #ifndef __PRETTY_FUNCTION__
        #ifdef __FUNCSIG__
            #define __PRETTY_FUNCTION__ __FUNCSIG__
        #elif defined(__FUNCDNAME__)
            #define __PRETTY_FUNCTION__ __FUNCDNAME__
        #else
            #define __PRETTY_FUNCTION__ __func__
        #endif
    #endif
#endif

#ifdef __KERNEL__
    extern void panic(const char*, ...);
    #include "Print.h"
    #define kprintf printf
    #define ksprintf sprintf
    #define ksnprintf snprintf
    #define kvsnprintf vsnprintf
    #define kvprintf vprintf
    #define PRINTF kprintf
    #define ABORT panic
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <errno.h>
    #define errorstr (strerror(errno))
    static inline void panic(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        abort();
    }
    #define PRINTF printf
    #define ABORT panic
#endif

#ifdef DEBUG
#define pinfo(fmt, ...) \
{                       \
    PRINTF("%s DInfo: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
};
#else
#define pinfo(fmt, ...) {}
#endif

#ifdef DEBUG
#define pinfof(fmt, ...) \
{                        \
    PRINTF("%s DInfo: " fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__); \
};
#else
#define pinfof(fmt, ...) {}
#endif

#ifdef DEBUG
#define perr(fmt, ...) \
{                      \
    PRINTF("%s Error: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
};
#else
#define perr(fmt, ...) {}
#endif

#define pwarning(fmt, ...) \
{                          \
    PRINTF("%s Warning: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
};

#define passert(cond, fmt, ...) \
{ \
    if (!(cond)) \
    { \
        PRINTF("%s Assertion Failed: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
        ABORT("Assertion triggered in file %s at line %d\n", __FILE__, __LINE__); \
    } \
};

#define psoftassert(cond, fmt, ...) \
{ \
    if (!(cond)) \
    { \
        PRINTF("%s Assertion Failed: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
        PRINTF("Assertion triggered in file %s at line %d\n", __FILE__, __LINE__); \
    } \
};

#define pfatal(fmt, ...) \
{ \
    ABORT("%s Fatal Error: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__); \
};

#endif /* Debug_h */
