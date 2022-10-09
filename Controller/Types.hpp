//
//  Number.hpp
//  Controller
//
//  Created by FireWolf on 9/29/20.
//  Copyright © 2020 FireWolf. All rights reserved.
//

#ifndef Number_hpp
#define Number_hpp

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Convenient number definitions

// An 8-bit unsigned integer value type.
typedef uint8_t UInt8;

// A 16-bit unsigned integer value type.
typedef uint16_t UInt16;

// A 32-bit unsigned integer value type.
typedef uint32_t UInt32;

// A 64-bit unsigned integer value type.
typedef uint64_t UInt64;

// A variable sized unsigned integer value type.
// On 64-bit system, it has the same size as UInt64.
// On 32-bit system, it has the same size as UInt32.
typedef size_t UInt;

// An 8-bit signed integer value type.
typedef int8_t SInt8;

// A 16-bit signed integer value type.
typedef int16_t SInt16;

// A 32-bit signed integer value type.
typedef int32_t SInt32;

// A 64-bit signed integer value type.
typedef int64_t SInt64;

// A variable sized signed integer value type.
// On 64-bit system, it has the same size as UInt64.
// On 32-bit system, it has the same size as UInt32.
typedef long SInt;

// A boolean value type
typedef bool Bool;

// A float value type
typedef float Float;

// A double value type
typedef double Double;

// A null-terminated C-string type
typedef const char* CString;

#endif /* Number_hpp */
