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

#include "quatification.h"
#include "ordre.h"
#include "imprimeur.h"
#include "traits.h"

#include "../chrono/chronometre_de_portee.hh"

#include <iostream>
#include <random>

using namespace dls::nombre_decimaux;

template <typename real>
void test_encode(std::ostream &os)
{
	constexpr auto NUM_REAL = 10;

	std::mt19937 rng(19937);
	std::uniform_real_distribution<real> dist(real(-0.5), real(0.5));

	for (int i = 0; i < NUM_REAL; ++i) {
		auto x = dist(rng);

		auto encoded = quantify<8>(x);
		auto decoded = dequantify<real>(encoded);

		int j = 0;
		for (;; ++j) {
			encoded = quantify<8>(x);
			decoded = dequantify<real>(encoded);

			if (std::abs(x - decoded) > 0.00001f || j == 10000) {
				break;
			}
		}

		os << x << " -> " << decoded << " (" << j << " iterations)\n";
	}
}

template <typename real>
void test_mul_div(std::ostream &os, char flags)
{
	real_printer<real> printer(os);
	printer.set_verbose_level(flags);

	auto d1 = real(7.97922663e16);
	auto d2 = d1;

	for (int i = 0; i < 30; ++i) {
		d1 /= real(7.0);
		d2 *= real(1.0 / 7.0);
	}

	printer(1.0);
	printer(d1);
	printer(d2);
}

/* This class represents a very limited high-precision number with 'count' 32-bit
 * unsigned elements. */
template <size_t count>
struct high_precision {
	typedef unsigned value_type;
	typedef unsigned long long product_type;
	static constexpr int m_word_shift = dls::math::nombre_bit<value_type>::valeur;

	/* The individual 'digits' (32-bit unsigned integers actually) that */
	/* make up the number. The most-significant digit is in m_data[0]. */
	value_type m_data[count];

	high_precision()
	{
		memset(m_data, 0, sizeof(m_data));
	}

	/* Insert the bits from value into m_data, shifted in from the bottom (least
	 * significant end) by the specified number of bits. A shift of zero or less
	 * means that none of the bits will be shifted in. A shift of one means that
	 * the high bit of value will be in the bottom of the last element of m_data -
	 * the least significant bit. A shift of kWordShift means that value will be
	 * in the least significant element of m_data, and so on.
	 */
	void insert_low_bits(value_type value, int shift_amount)
	{
		if (shift_amount <= 0) {
			return;
		}

		const auto sub_shift = shift_amount & (m_word_shift - 1);
		const auto big_shift = shift_amount / m_word_shift;
		const auto result = product_type(value) << sub_shift;
		const auto result_low = static_cast<value_type>(result);
		const auto result_high = result >> m_word_shift;

		/* Use an unsigned type so that negative numbers will become large,
		 * which makes the range checking below simpler. */
		const auto high_index = count - 1 - big_shift;

		/* Write the results to the data array. If the index is too large then
		 * that means that the data was shifted off the edge. */
		if (high_index < count) {
			m_data[high_index] |= result_high;
		}

		if (high_index + 1 < count){
			m_data[high_index + 1] |= result_low;
		}
	}

	/* Insert the bits from value into m_data, shifted in from the top (most
	 * significant end) by the specified number of bits. A shift of zero or less
	 * means that none of the bits will be shifted in. A shift of one means that
	 * the low bit of value will be in the top of the first element of m_data -
	 * the most significant bit. A shift of kWordShift means that value will be
	 * in the most significant element of m_data, and so on.
	 */
	void insert_top_bits(value_type value, int shift_amount)
	{
		insert_low_bits(value, (count + 1) * m_word_shift - shift_amount);
	}

	/* Return true if all elements of m_data are zero. */
	bool is_zero() const
	{
		for (int i(0); i < count; ++i) {
			if (m_data[i])
				return false;
		}

		return true;
	}

	/* Divide by div and return the remainder, from 0 to div-1. */
	value_type remainder(value_type divisor)
	{
		value_type remain(0);

		/* Standard long-division algorithm. */
		for (int i = 0; i < count; ++i) {
			auto dividend = (product_type(remain) << m_word_shift) + m_data[i];
			auto result = dividend / divisor;
			remain = value_type(dividend % divisor);
			m_data[i] = value_type(result);
		}

		return remain;
	}

	/* Multiply by mul and return the overflow, from 0 to mul - 1 */
	value_type overflow(value_type mul)
	{
		value_type over(0);

		for (int i = count - 1; i >= 0; --i) {
			auto result = product_type(mul) * m_data[i] + over;

			/* Put the bottom bits of the results back. */
			m_data[i] = value_type(result);
			over = value_type(result >> m_word_shift);
		}

		return over;
	}
};

int main()
{
	std::ostream &os = std::cout;

	CHRONOMETRE_PORTEE(__func__, os);

#if 0
	auto f = 1.0;
	auto i = as_integral(f);

	std::cout << "float as int: " << f << " -> " << i << "\n";

	f = as_floating_point(i);

	os << "int as float: " << i << " -> " << f << "\n";

	auto nf = make_signaling_nan<decltype(f)>(1);

	if (is_positive_nan(nf)) {
		os << "nf is positive nan\n";
	}
	else if (is_negative_nan(nf)) {
		os << "nf is negative nan\n";
	}

	if (is_ulp_equal(1.0, 1.000001, 10)) {
		os << "ulp equal\n";
	}

	if (total_equal(f, nf)) {
		std::cerr << "Major fuck up...\n";
		return 1;
	}

	os << "epsilon: " << std::numeric_limits<decltype(f)>::epsilon() << "\n";
	os << "tolerance: " << tolerance<decltype(f)>::value() << "\n";
	os << "delta: " << delta<decltype(f)>::value() << "\n";

#endif

	test_encode<float>(os);
	/* ******************** real_printer ************************* */

#if 0
	char flags = PRINT_VALUE_PASSED | PRINT_AS_INTEGER | PRINT_FORMULA | PRINT_BITS_RAW;

	real_printer<float> float_printer(os);
	float_printer.set_verbose_level(flags);

	float_printer(5.0f);
	float_printer(-29.24f);
	float_printer(29.24f);
	float_printer(0.1f);
	float_printer(std::numeric_limits<float>::max());

	real_printer<double> double_printer(os);
	double_printer.set_verbose_level(flags);

	double_printer(3.14159265358979323846);
	double_printer(0.78539816339744830962);
	double_printer(std::numeric_limits<double>::max());

	test_mul_div<float>(os, flags);
	test_mul_div<double>(os, flags);
#endif
}
