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

#include "traits.h"

#include "tableau_precision.h"

#include "../flux/outils.h"
#include "../math/outils.hh"

namespace dls {
namespace nombre_decimaux {

namespace alg {

template <typename Integer>
void add_with_carry(const Integer *first1, const Integer *last1, Integer *last2)
{
	Integer tmp_result;
	Integer carry = 0;

	while (last1 >= first1) {
		tmp_result = *last1 + *last2 + carry;
		carry = 0;

		if (tmp_result >= 10) {
			tmp_result -= 10;
			carry = 1;
		}

		*last2 = tmp_result;
		--last1;
		--last2;
	}
}

template <typename Integer>
void multiply_with_carry(const Integer *first, const Integer *last, Integer *result)
{
	Integer carry(0);

	while (last >= first) {
		Integer cur = (*last << 1) + carry;
		carry = 0;

		while (cur >= 10) {
			cur -= 10;
			carry += 1;
		}

		*result = cur;

		--last;
		--result;
	}
}

template <typename Integer>
void divide_with_carry(const Integer *first, const Integer *last, Integer *result)
{
	while (first < last) {
		Integer cur = *first;

		if (cur != 0) {
			if ((cur & 1) == 1) {
				*(result + 1) = 5;
			}

			*result = (cur >> 1) + *result;
		}

		++first;
		++result;
	}
}

}  /* namespace alg */

namespace bits {

template <typename Decimal>
void copy_bits_binary(Decimal decimal, char *first, char *last)
{
	Decimal i(0);

	while (last >= first) {
		*last-- = '0' + ((decimal & (Decimal(1) << i)) >> i);
		++i;
	}
}

}  /* namespace bits */

enum {
	PRINT_VALUE_PASSED  = (1 << 0),
	PRINT_VALUE_ACTUAL  = (1 << 1),
	PRINT_BITS_RAW      = (1 << 2),
	PRINT_BITS_SEPARATE = (1 << 3),
	PRINT_FORMULA       = (1 << 4),
	PRINT_AS_INTEGER    = (1 << 5),

	PRINT_ALL = (PRINT_VALUE_PASSED | PRINT_VALUE_ACTUAL | PRINT_BITS_RAW |
	             PRINT_BITS_SEPARATE | PRINT_FORMULA | PRINT_AS_INTEGER),
};

/**
 * A helper class to print informations about a floating-point number. The infos
 * are the actual bit representation, the value an integer would hold with the
 * same bit representation, as well as the IEEE 'formula' for that number.
 *
 * Example output for double(pi):
 * Value: 3.14159
 * Raw Bits: 0100000000001001001000011111101101010100010001000010110100011000
 * Sign: 0
 * Exponent: 10000000000
 * Mantissa: 1001001000011111101101010100010001000010110100011000
 * As integer: 4614256656552045848
 * IEEE formula: 1.5707963267948965579989817342720925807952880859375 * 2e1
 */
template <typename real>
class real_printer {
	using traits = floating_point_info<real>;

	static constexpr size_t NBITS = math::nombre_bit<real>::valeur;
	static constexpr int EXP      = traits::EXP;
	static constexpr int EXP_MASK = traits::EXP_MASK;
	static constexpr int EXP_BIAS = traits::EXP_BIAS;
	static constexpr int MANT     = traits::MANT;

	std::ostream &m_os;
	char m_verbose_level;

public:
	explicit real_printer(std::ostream &os);

	void operator()(real f);
	void set_verbose_level(const char verbose);

private:
	void mantissa_from_bits(const char *bits, char *result);
};

template <typename real>
real_printer<real>::real_printer(std::ostream &os)
    : m_os(os)
    , m_verbose_level(PRINT_ALL)
{}

template <typename real>
void real_printer<real>::operator()(real f)
{
	if ((m_verbose_level & PRINT_VALUE_PASSED) != 0) {
		m_os << "Value: " << f << '\n';
	}

	auto i = as_integral(f);
	char binaire[NBITS];

	bits::copy_bits_binary(i, &binaire[0], &binaire[NBITS - 1]);

	if ((m_verbose_level & PRINT_BITS_RAW) != 0) {
		flux::print_array(&binaire[0], &binaire[NBITS], m_os, "Raw Bits: ", "\n");
	}

	if ((m_verbose_level & PRINT_BITS_SEPARATE) != 0) {
		flux::print_array(&binaire[0], &binaire[1], m_os, "Sign: ", "\n");
		flux::print_array(&binaire[1], &binaire[EXP + 1], m_os, "Exponent: ", "\n");
		flux::print_array(&binaire[EXP + 1], &binaire[EXP + MANT + 1], m_os, "Mantissa: ", "\n");
	}

	if ((m_verbose_level & PRINT_AS_INTEGER) != 0) {
		m_os << "As integer: " << i << '\n';
	}

	auto exp = exponent(f) - EXP_BIAS;// int(((i >> MANT) & EXP_MASK) - EXP_BIAS);

	if ((m_verbose_level & PRINT_FORMULA) != 0) {
		char mantissa[53] = { 0 };
		mantissa_from_bits(&binaire[EXP + 1], &mantissa[0]);
		flux::format_number_array(&mantissa[0], &mantissa[52]);

		m_os << "IEEE formula: ";
		m_os << ((binaire[0] == '1') ? "-" : "+");
		m_os << "1." << mantissa;
		m_os << " * 2e" << exp << '\n';
	}

	if ((m_verbose_level & PRINT_VALUE_ACTUAL) != 0) {
		char actual[54] = { 0 };
		mantissa_from_bits(&binaire[EXP + 1], &actual[1]);
		actual[0] = 1;

		bool implemented = true;

		if (exp < 0) {
			for (int ie = 0; ie < (-exp); ++ie) {
				char tmp[54] = { 0 };
				alg::divide_with_carry(&actual[0], &actual[53], &tmp[0]);

				for (int j = 0; j < 54; ++j) {
					actual[j] = tmp[j];
				}
			}
		}
		else if ((exp > 0) && (exp < 3)) {
			// TODO: exponent > 2
			for (int ie = 0; ie < exp; ++ie) {
				char tmp[54] = { 0 };
				alg::multiply_with_carry(&actual[0], &actual[53], &tmp[53]);

				for (int j = 0; j < 54; ++j) {
					actual[j] = tmp[j];
				}
			}
		}
		else if (exp > 2) {
			implemented = false;
		}

		m_os << "Actual Value: ";

		if (implemented) {
			flux::format_number_array(&actual[0], &actual[53]);
			m_os << actual[0] << ".";
			flux::print_array(&actual[1], &actual[53], m_os);
		}
		else {
			m_os << "todo, not implemented.";
		}

		m_os << "\n";
	}

	m_os << '\n';
}

template <typename real>
void real_printer<real>::mantissa_from_bits(const char *bits, char *result)
{
	using alg::add_with_carry;

	for (size_t i = 0; i < MANT; ++i) {
		if (bits[i] == '1') {
			add_with_carry(significand_precision[i],
			               significand_precision[i] + (MANT - 1),
			               result + (MANT - 1));
		}
	}
}

template <typename real>
void real_printer<real>::set_verbose_level(const char verbose)
{
	m_verbose_level = verbose;
}

}  /* namespace nombre_decimaux */
}  /* namespace dls */
