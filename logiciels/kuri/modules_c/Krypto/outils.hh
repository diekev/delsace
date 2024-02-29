/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include <cstdint>

// big endian architectures need #define __BYTE_ORDER __BIG_ENDIAN
#ifndef _MSC_VER
#    include <endian.h>
#endif

/* ------------------------------------------------------------------------- */
/** \name Conversion boutisme.
 * \{ */

#if defined(__BYTE_ORDER) && (__BYTE_ORDER != 0) && (__BYTE_ORDER == __BIG_ENDIAN)
#    define LITTLEENDIAN(x) swap(x)
#else
#    define LITTLEENDIAN(x) (x)
#endif

/// convert litte vs big endian
inline uint32_t swap(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(x);
#endif
#ifdef MSC_VER
    return _byteswap_ulong(x);
#endif

    return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24);
}

/// convert litte vs big endian
inline uint64_t swap(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(x);
#endif
#ifdef _MSC_VER
    return _byteswap_uint64(x);
#endif

    return (x >> 56) | ((x >> 40) & 0x000000000000FF00ULL) | ((x >> 24) & 0x0000000000FF0000ULL) |
           ((x >> 8) & 0x00000000FF000000ULL) | ((x << 8) & 0x000000FF00000000ULL) |
           ((x << 24) & 0x0000FF0000000000ULL) | ((x << 40) & 0x00FF000000000000ULL) | (x << 56);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Mélange bits.
 * \{ */

inline uint32_t rotate(uint32_t a, uint32_t c)
{
    return (a << c) | (a >> (32 - c));
}

/// rotate left and wrap around to the right
inline uint64_t rotateLeft(uint64_t x, uint8_t numBits)
{
    return (x << numBits) | (x >> (64 - numBits));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Autres.
 * \{ */

/// return x % 5 for 0 <= x <= 9
inline uint32_t mod5(uint32_t x)
{
    if (x < 5)
        return x;

    return x - 5;
}

/** \} */
