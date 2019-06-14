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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cassert>
#include <emmintrin.h>

class sse_float {
	union {
		__m128 m_128;
		float m_f[4];
	};

	enum {
		size = 4
	};

public:
	/* ************ constructions ************* */

	inline sse_float()
	    : m_128(_mm_setzero_ps())
	{}

	inline sse_float(float a)
	    :  m_128(_mm_set1_ps(a))
	{}

	inline sse_float(float *a)
	    :  m_128(_mm_loadu_ps(a))
	{}

	inline sse_float(const __m128 a)
	    : m_128(a)
	{}

	inline sse_float &operator=(const sse_float &other)
	{
		m_128 = other.m_128;
		return *this;
	}

	inline operator const __m128 &() const { return m_128; }
	inline operator       __m128 &()       { return m_128; }

	inline sse_float(float a, float b, float c, float d)
	    : m_128(_mm_setr_ps(a, b, c, d))
	{}

	inline explicit sse_float(const __m128i a)
	    : m_128(_mm_cvtepi32_ps(a))
	{}

	inline const float& operator [](const size_t i) const { assert(i < 4); return m_f[i]; }
	inline       float& operator [](const size_t i)       { assert(i < 4); return m_f[i]; }

	/* ************ operators ************* */

	inline const sse_float &operator+=(const sse_float &other)
	{
		m_128 = _mm_add_ps(m_128, other.m_128);
		return *this;
	}

	inline const sse_float &operator*=(const sse_float &other)
	{
		m_128 = _mm_mul_ps(m_128, other.m_128);
		return *this;
	}

	inline const sse_float &operator*=(const float f)
	{
		m_128 = _mm_mul_ps(m_128, _mm_set1_ps(f));
		return *this;
	}
};

sse_float operator*(const sse_float &sf, const float f)
{
	sse_float tmp(sf);
	tmp *= f;
	return tmp;
}
