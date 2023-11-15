/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

/**
 * Type nombre réel sur 16-bits inspiré des types half de PTEX et OpenEXR.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
/* bool est défini dans stdbool.h */
#ifndef bool
typedef unsigned char bool;
#endif
typedef unsigned short  r16;
#endif

typedef float  r32;
typedef double r64;

typedef unsigned short n16;
typedef unsigned int   n32;

typedef short z16;
typedef int   z32;

#include "r16_tables.h"

/* ************* conversions ************* */

static inline r32 DLS_vers_r32(n16 h)
{
	union { n32 i; float f; } u;
	u.i = table_r16_r32[h];
	return u.f;
}

static inline r64 DLS_vers_r64(n16 a)
{
	return (r64)(DLS_vers_r32(a));
}

static inline r32 DLS_fabs(r32 f)
{
	return (f < 0.0f) ? -f : f;
}

static inline n16 DLS_depuis_r32_except(n32 i)
{
	n32 s = ((i>>16) & 0x8000);
	z32 e = (z32)(((i>>13) & 0x3fc00) - 0x1c000);

	if (e <= 0) {
		// denormalized
		union { n32 i; r32 f; } u;
		u.i = i;
		return (n16)(s | (n32)(DLS_fabs(u.f) * 1.6777216e7f + 0.5f));
	}

	if (e == 0x23c00) {
		// inf/nan, preserve msb bits of m for nan code
		return (n16)(s|0x7c00|((i&0x7fffff)>>13));
	}

	// overflow - convert to inf
	return (n16)(s | 0x7c00);
}

static inline n16 DLS_depuis_r32(r32 val)
{
	if (val == 0.0f) {
		return 0;
	}

	union { n32 i; float f; } u;
	u.f = val;

	n32 e = table_r32_r16[(u.i >> 23) & 0x1ff];

	if (e) {
		return (n16)(e + (((u.i & 0x7fffff) + 0x1000) >> 13));
	}

	return DLS_depuis_r32_except(u.i);
}

static inline n16 DLS_depuis_r64(r64 v)
{
	return DLS_depuis_r32((float)(v));
}

/* ************* opérateurs unaires ************* */

static inline n16 DLS_plus_r16(n16 a)
{
	return a;
}

static inline n16 DLS_moins_r16(n16 a)
{
	r32 f = DLS_vers_r32(a);
	return DLS_depuis_r32(-f);
}

/* ************* addition ************* */

static inline n16 DLS_ajoute_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return DLS_depuis_r32(f1 + f2);
}

static inline n16 DLS_ajoute_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return DLS_depuis_r32(f1 + b);
}

static inline n16 DLS_ajoute_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return DLS_depuis_r32(a + f1);
}

static inline n16 DLS_ajoute_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return DLS_depuis_r64(f1 + b);
}

static inline n16 DLS_ajoute_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return DLS_depuis_r64(a + f1);
}

/* ************* soustraction ************* */

static inline n16 DLS_soustrait_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return DLS_depuis_r32(f1 - f2);
}

static inline n16 DLS_soustrait_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return DLS_depuis_r32(f1 - b);
}

static inline n16 DLS_soustrait_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return DLS_depuis_r32(a - f1);
}

static inline n16 DLS_soustrait_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return DLS_depuis_r64(f1 - b);
}

static inline n16 DLS_soustrait_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return DLS_depuis_r64(a - f1);
}

/* ************* multiplication ************* */

static inline n16 DLS_multiplie_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return DLS_depuis_r32(f1 * f2);
}

static inline n16 DLS_multiplie_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return DLS_depuis_r32(f1 * b);
}

static inline n16 DLS_multiplie_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return DLS_depuis_r32(a * f1);
}

static inline n16 DLS_multiplie_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return DLS_depuis_r64(f1 * b);
}

static inline n16 DLS_multiplie_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return DLS_depuis_r64(a * f1);
}

/* ************* division ************* */

static inline n16 DLS_divise_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return DLS_depuis_r32(f1 / f2);
}

static inline n16 DLS_divise_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return DLS_depuis_r32(f1 / b);
}

static inline n16 DLS_divise_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return DLS_depuis_r32(a / f1);
}

static inline n16 DLS_divise_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return DLS_depuis_r64(f1 / b);
}

static inline n16 DLS_divise_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return DLS_depuis_r64(a / f1);
}

/* ************* comparaisons ************* */

static inline bool DLS_compare_inf_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 < f2;
}

static inline bool DLS_compare_inf_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 < b;
}

static inline bool DLS_compare_inf_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a < f1;
}

static inline bool DLS_compare_inf_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 < b;
}

static inline bool DLS_compare_inf_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a < f1;
}

static inline bool DLS_compare_inf_egl_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 <= f2;
}

static inline bool DLS_compare_inf_egl_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 <= b;
}

static inline bool DLS_compare_inf_egl_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a <= f1;
}

static inline bool DLS_compare_inf_egl_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 <= b;
}

static inline bool DLS_compare_inf_egl_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a <= f1;
}

static inline bool DLS_compare_sup_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 > f2;
}

static inline bool DLS_compare_sup_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 > b;
}

static inline bool DLS_compare_sup_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a > f1;
}

static inline bool DLS_compare_sup_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 > b;
}

static inline bool DLS_compare_sup_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a > f1;
}

static inline bool DLS_compare_sup_egl_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 >= f2;
}

static inline bool DLS_compare_sup_egl_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 >= b;
}

static inline bool DLS_compare_sup_egl_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a >= f1;
}

static inline bool DLS_compare_sup_egl_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 >= b;
}

static inline bool DLS_compare_sup_egl_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a >= f1;
}

static inline bool DLS_compare_egl_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 == f2;
}

static inline bool DLS_compare_egl_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 == b;
}

static inline bool DLS_compare_egl_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a == f1;
}

static inline bool DLS_compare_egl_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 == b;
}

static inline bool DLS_compare_egl_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a == f1;
}

static inline bool DLS_compare_non_egl_r16r16(n16 a, n16 b)
{
	r32 f1 = DLS_vers_r32(a);
	r32 f2 = DLS_vers_r32(b);
	return f1 != f2;
}

static inline bool DLS_compare_non_egl_r16r32(n16 a, r32 b)
{
	r32 f1 = DLS_vers_r32(a);
	return f1 != b;
}

static inline bool DLS_compare_non_egl_r32r16(r32 a, n16 b)
{
	r32 f1 = DLS_vers_r32(b) ;
	return a != f1;
}

static inline bool DLS_compare_non_egl_r16r64(n16 a, r64 b)
{
	r64 f1 = DLS_vers_r64(a);
	return f1 != b;
}

static inline bool DLS_compare_non_egl_r64r16(r64 a, n16 b)
{
	r64 f1 = DLS_vers_r64(b);
	return a != f1;
}

/* ************* autres ************* */

static inline bool	DLS_est_finie_r16(n16 a)
{
	unsigned short e = (a >> 10) & 0x001f;
	return e < 31;
}

static inline bool DLS_est_normalisee_r16(n16 a)
{
	unsigned short e = (a >> 10) & 0x001f;
	return e > 0 && e < 31;
}

static inline bool DLS_est_denormalisee_r16(n16 a)
{
	unsigned short e = (a >> 10) & 0x001f;
	unsigned short m =  a & 0x3ff;
	return e == 0 && m != 0;
}

static inline bool DLS_est_zero_r16(n16 a)
{
	return (a & 0x7fff) == 0;
}

static inline bool DLS_est_nan_r16(n16 a)
{
	unsigned short e = (a >> 10) & 0x001f;
	unsigned short m =  a & 0x3ff;
	return e == 31 && m != 0;
}

static inline bool DLS_est_infinie_r16(n16 a)
{
	unsigned short e = (a >> 10) & 0x001f;
	unsigned short m =  a & 0x3ff;
	return e == 31 && m == 0;
}

static inline bool DLS_est_negative_r16(n16 a)
{
	return (a & 0x8000) != 0;
}

static inline n16 DLS_r16_infinie_positive()
{
	return 0x7c00;
}

static inline n16 DLS_r16_infinie_negative()
{
	return 0xfc00;
}

static inline n16 DLS_r16_nan_q()
{
	return 0x7fff;
}

static inline n16 DLS_r16_nan_s()
{
	return 0x7dff;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
