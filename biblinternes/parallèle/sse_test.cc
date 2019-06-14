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

#include "sse_float.hh"
#include "sse_test.hh"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>
#include <x86intrin.h>

#include "../chrono/chronometre_de_portee.hh"

class saxpy_kernel {
	float m_a;

public:
	explicit saxpy_kernel(float a)
	    : m_a(a)
	{}

	enum {
		BLOCK_SIZE = 8
	};

	inline void operator()(float *y, float *x) const
	{
		*y = m_a * *x;
	}

	inline void block(float *y, float *x) const
	{
		sse_float X0 = x;
		sse_float X4 = x + 4;
		X0 *= m_a;
		X4 *= m_a;
		_mm_storeu_ps(y, X0);
		_mm_storeu_ps(y + 4, X4);
	}
};

float accumulate(std::vector<float> &v)
{
	// copy the length of v and a pointer to the data onto the local stack
	const size_t n = v.size();

	if (n == 0ul) {
		return 0.0f;
	}

	float *p = &v.front();

	size_t i = 0ul, e = ROUND_DOWN(n, 4ul);

#if 0
	__m128 mmSum = _mm_setzero_ps();

	// unrolled loop that adds up 4 elements at a time
	for (; i < e; i += 4ul) {
		mmSum = _mm_add_ps(mmSum, _mm_load_ps(p + i));
	}

	for (; i < n; ++i) {
		mmSum = _mm_add_ps(mmSum, _mm_load_ps(p + i));
	}

	mmSum = _mm_add_ps(mmSum, mmSum);
	mmSum = _mm_add_ps(mmSum, mmSum);

	return _mm_cvtss_f32(mmSum);
#else
	sse_float mmSum;

	// unrolled loop that adds up 4 elements at a time
	for (; i < e; i += 4ul) {
		mmSum += sse_float(p + i);
	}

	for (; i < n; ++i) {
		mmSum = _mm_add_ss(mmSum, _mm_load_ss(p + i));
	}

	mmSum = _mm_hadd_ps(mmSum, mmSum);
	mmSum = _mm_hadd_ps(mmSum, mmSum);

	return _mm_cvtss_f32(mmSum);
#endif
}

auto test_sse()
{
#if 0
	constexpr auto size = 500000;
	constexpr auto iter = 5000;
	std::vector<float> v(size), v2(size);
	std::iota(v.begin(), v.end(), 0.0f);
	std::iota(v2.begin(), v2.end(), 1.0f);

	std::vector<float> sv(size), sv2(size);
	std::iota(sv.begin(), sv.end(), 0.0f);
	std::iota(sv2.begin(), sv2.end(), 1.0f);

	for (size_t i = 0; i < size; ++i) {
		if (v2[i] != sv2[i]) {
			std::cout << "Vectors are different! (index: " << i << ")\n";
			std::cout << "buit-in: " << v2[i] << ", sse: " << sv2[i] << "\n";
			break;
		}
	}

	float d = 2;
	{
		CHRONOMETRE_PORTEE("saxpy", std::cout);

		for (int i = 0; i < iter; ++i) {
			transform_n(&v2[0], &v[0], size, saxpy_kernel(d));
		}
	}
	{
		CHRONOMETRE_PORTEE("saxpy sse", std::cout);

		for (int i = 0; i < iter; ++i) {
			transform_n_sse(&sv2[0], &sv[0], size, saxpy_kernel(d));
		}
	}

	for (size_t i = 0; i < size; ++i) {
		if (v2[i] != sv2[i]) {
			std::cout << "Vectors are different! (index: " << i << ")\n";
			std::cout << "buit-in: " << v2[i] << ", sse: " << sv2[i] << "\n";
			break;
		}
	}
#else
	std::vector<float> v(1000);
	std::iota(v.begin(), v.end(), 0.0f);

	std::cout << "sse: " << accumulate(v) << "\n";
	std::cout << "builtin: " << std::accumulate(v.begin(), v.end(), 0.0f) << "\n";
#endif
}

