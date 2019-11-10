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

#include <pmmintrin.h>

#include "biblinternes/moultfilage/sse_mathfun.hh"

struct sse_r32 {
private:
	union {
		__m128 m_128;
		float m_f[4];
	};

	enum {
		size = 4
	};

public:
	/* construction */

	inline sse_r32()
		: m_128(_mm_setzero_ps())
	{}

	inline sse_r32(float a)
		: m_128(_mm_set1_ps(a))
	{}

	inline sse_r32(float const *a)
		: m_128(_mm_load_ps(a))
	{}

	inline sse_r32(float a, float b, float c, float d)
		: m_128(_mm_setr_ps(a, b, c, d))
	{}

	inline sse_r32(__m128 a)
		: m_128(a)
	{}

	/* opération */

	inline operator const __m128 &() const { return m_128; }
	inline operator       __m128 &()       { return m_128; }

	inline auto operator+=(const sse_r32 &a)
	{
		m_128 = _mm_add_ps(m_128, a.m_128);
		return *this;
	}

	inline auto operator+=(float a)
	{
		m_128 = _mm_add_ps(m_128, _mm_set_ps1(a));
		return *this;
	}

	inline auto operator-=(const sse_r32 &a)
	{
		m_128 = _mm_sub_ps(m_128, a.m_128);
		return *this;
	}

	inline auto operator-=(float a)
	{
		m_128 = _mm_sub_ps(m_128, _mm_set_ps1(a));
		return *this;
	}

	inline auto operator*=(const sse_r32 &a)
	{
		m_128 = _mm_mul_ps(m_128, a.m_128);
		return *this;
	}

	inline auto operator*=(float a)
	{
		m_128 = _mm_mul_ps(m_128, _mm_set_ps1(a));
		return *this;
	}

	inline auto operator/=(const sse_r32 &a)
	{
		m_128 = _mm_div_ps(m_128, a.m_128);
		return *this;
	}

	inline auto operator/=(float a)
	{
		m_128 = _mm_div_ps(m_128, _mm_set_ps1(a));
		return *this;
	}
};

inline auto operator+(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_add_ps(a, b));
}

inline auto operator-(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_sub_ps(a, b));
}

inline auto operator*(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_mul_ps(a, b));
}

inline auto operator/(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_div_ps(a, b));
}

inline auto operator<(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmplt_ps(a, b));
}

inline auto operator<=(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmple_ps(a, b));
}

inline auto operator>(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmpgt_ps(a, b));
}

inline auto operator>=(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmpge_ps(a, b));
}

inline auto operator==(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmpeq_ps(a, b));
}

inline auto operator!=(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_cmpneq_ps(a, b));
}

inline auto aplani_ajoute(sse_r32 const &a)
{
	auto r = a;
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	return _mm_cvtss_f32(r);
}

inline auto et_binaire(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_and_ps(a, b));
}

inline auto etnon_binaire(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_andnot_ps(a, b));
}

inline auto ou_binaire(sse_r32 const &a, sse_r32 const &b)
{
	return sse_r32(_mm_or_ps(a, b));
}

inline auto exp(sse_r32 const &a)
{
	return sse_r32(exp_ps(a));
}

inline auto log(sse_r32 const &a)
{
	return sse_r32(log_ps(a));
}

inline auto sqrt(sse_r32 const &a)
{
	return sse_r32(_mm_sqrt_ps(a));
}
