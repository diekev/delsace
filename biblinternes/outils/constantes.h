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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cmath>

template <typename T>
struct constantes;

template <>
struct constantes<float> {
	static constexpr auto E         = 2.7182818284590452354f; /* e */
	static constexpr auto LOG2E     = 1.4426950408889634074f;   /* log_2 e */
	static constexpr auto LOG10E    = 0.43429448190325182765f;	/* log_10 e */
	static constexpr auto LN2       = 0.69314718055994530942f;	/* log_e 2 */
	static constexpr auto LN10      = 2.30258509299404568402f;	/* log_e 10 */

	static constexpr auto PI        = 3.14159265358979323846f;	/* pi */
	static constexpr auto PI_2      = 1.57079632679489661923f;	/* pi/2 */
	static constexpr auto PI_4      = 0.78539816339744830962f;	/* pi/4 */
	static constexpr auto PI_INV     = 0.31830988618379067154f;	/* 1/pi */
	static constexpr auto PI_INV_2     = 0.63661977236758134308f;	/* 2/pi */
	static constexpr auto SQRTPI_INV_2 = 1.12837916709551257390f;	/* 2/sqrt(pi) */
	static constexpr auto SQRT2     = 1.41421356237309504880f;	/* sqrt(2) */
	static constexpr auto SQRT1_2   = 0.70710678118654752440f;	/* 1/sqrt(2) */
	static constexpr auto TAU     = 2.0f * PI;
	static constexpr auto TAU_INV = 1.0f / TAU;
	static constexpr auto PHI     = 1.6180339887498948482f;
	static constexpr auto PHI_INV = 0.6180339887498948482f;
	static constexpr auto POIDS_DEG_RAD = PI / 180.0f;
	static constexpr auto POIDS_RAD_DEG = 180.0f / PI;
	static constexpr auto INFINITE  = std::numeric_limits<float>::max();
};

template <>
struct constantes<double> {
	static constexpr auto E         = 2.7182818284590452354;   /* e */
	static constexpr auto LOG2E     = 1.4426950408889634074;   /* log_2 e */
	static constexpr auto LOG10E    = 0.43429448190325182765;  /* log_10 e */
	static constexpr auto LN2       = 0.69314718055994530942;  /* log_e 2 */
	static constexpr auto LN10      = 2.30258509299404568402;  /* log_e 10 */
	static constexpr auto PI        = 3.14159265358979323846;  /* pi */
	static constexpr auto PI_2      = 1.57079632679489661923;  /* pi/2 */
	static constexpr auto PI_4      = 0.78539816339744830962;  /* pi/4 */
	static constexpr auto PI_INV     = 0.31830988618379067154;  /* 1/pi */
	static constexpr auto PI_INV_2     = 0.63661977236758134308;  /* 2/pi */
	static constexpr auto SQRTPI_INV_2 = 1.12837916709551257390;  /* 2/sqrt(pi) */
	static constexpr auto SQRT2     = 1.41421356237309504880;  /* sqrt(2) */
	static constexpr auto SQRT1_2   = 0.70710678118654752440;  /* 1/sqrt(2) */
	static constexpr auto TAU     = 2.0 * PI;
	static constexpr auto TAU_INV = 1.0 / TAU;
	static constexpr auto PHI     = 1.6180339887498948482;
	static constexpr auto PHI_INV = 0.6180339887498948482;
	static constexpr auto POIDS_DEG_RAD = PI / 180.0;
	static constexpr auto POIDS_RAD_DEG = 180.0 / PI;
	static constexpr auto INFINITE  = std::numeric_limits<double>::max();
};
