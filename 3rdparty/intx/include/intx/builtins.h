// builtins: Portable C/C++ compiler builtins.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

/**
 * @file
 * Implementation of GCC/clang builtins for MSVC compiler.
 */

#pragma once

#ifdef _MSC_VER
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the number of leading 0-bits in `x`, starting at the most significant bit position.
 * If `x` is 0, the result is undefined.
 */
static inline int __builtin_clz(unsigned int x)
{
    unsigned long most_significant_bit;
    _BitScanReverse(&most_significant_bit, x);
    return 31 ^ (int)most_significant_bit;
}

/**
 * Returns the number of leading 0-bits in `x`, starting at the most significant bit position.
 * If `x` is 0, the result is undefined.
 */
static inline int __builtin_clzl(unsigned __int64 x)
{
    unsigned long most_significant_bit;
    _BitScanReverse64(&most_significant_bit, x);
    return 63 ^ (int)most_significant_bit;
}

/**
 * Returns the number of 1-bits in `x`.
 */
static inline int __builtin_popcount(unsigned int x)
{
    return (int)__popcnt(x);
}

#ifdef __cplusplus
}
#endif

#endif

#ifdef __cplusplus
namespace builtins
{
inline int clz(unsigned int x)
{
    return __builtin_clz(x);
}
inline int clz(unsigned long x)
{
    return __builtin_clzl(x);
}
}  // namespace builtins
#endif
