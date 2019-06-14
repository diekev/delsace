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
#include <cmath>   /* for std::isnan */
#include <cstring> /* for memcpy */

#ifdef HALF_SUPPORT
#  include <OpenEXR/half.h>
#endif

namespace dls {
namespace nombre_decimaux {

/**
 * Helper structs to ensure we use the right integral type for a given floating-
 * point type, et vice versa.
 */

template <typename T>
struct integral_repr;

#ifdef HALF_SUPPORT
template <>
struct integral_repr<half> {
	using type = short;
};
#endif

template <>
struct integral_repr<float> {
	using type = int;
};

template <>
struct integral_repr<double> {
	using type = long;
};

template <typename T>
struct floating_repr;

#ifdef HALF_SUPPORT
template <>
struct floating_repr<short> {
	using type = half;
};
#endif

template <>
struct floating_repr<int> {
	using type = float;
};

template <>
struct floating_repr<long> {
	using type = double;
};

/**
 * Helper struct to hold information about the representation of a floating-point
 * number as well as what integral type to use for e.g. bitwise operations.
 */

template <typename real>
struct floating_point_info;

#ifdef HALF_SUPPORT
template <>
struct floating_point_info<half> {
	/* Actual type. */
	using type = half;
	/* Type used to interpret the bits as integer. */
	using integer_t = typename integral_repr<type>::type;

	/* Total number of bits used to store the exponent. */
	static constexpr int EXP = 5;
	/* Mask used to extract the exponent. */
	static constexpr integer_t EXP_MASK = 0x1f;
	/* Value to subract the exponent with. */
	static constexpr int EXP_BIAS = 0xf;

	/* Number of bits containing the sign bit + exponent bits. */
	static constexpr int HEADER = EXP + 1;
	/* Mask used to extract the header. */
	static constexpr integer_t HEADER_MASK = 0x3f;

	/* Total number of bits used to store the mantissa. */
	static constexpr int MANT = 10;
};
#endif

template <>
struct floating_point_info<float> {
	/* Actual type. */
	using type = float;
	/* Type used to interpret the bits as integer. */
	using integer_t = typename integral_repr<type>::type;

	/* Total number of bits used to store the exponent. */
	static constexpr int EXP = 8;
	/* Mask used to extract the exponent. */
	static constexpr integer_t EXP_MASK = 0xff;
	/* Value to subract the exponent with. */
	static constexpr int EXP_BIAS = 0x7f;

	/* Number of bits containing the sign bit + exponent bits. */
	static constexpr int HEADER = EXP + 1;
	/* Mask used to extract the header. */
	static constexpr integer_t HEADER_MASK = 0x1ff;

	/* Total number of bits used to store the mantissa. */
	static constexpr int MANT = 23;
};

template <>
struct floating_point_info<double> {
	/* Actual type. */
	using type = double;
	/* Type used to interpret the bits as integer. */
	using integer_t = typename integral_repr<type>::type;

	/* Total number of bits used to store the exponent. */
	static constexpr int EXP = 11;
	/* Mask used to extract the exponent. */
	static constexpr integer_t EXP_MASK = 0x7ff;
	/* Value to subract the exponent with. */
	static constexpr int EXP_BIAS = 0x3ff;

	/* Number of bits containing the sign bit + exponent bits. */
	static constexpr int HEADER = EXP + 1;
	/* Mask used to extract the header. */
	static constexpr integer_t HEADER_MASK = 0xfff;

	/* Total number of bits used to store the mantissa. */
	static constexpr int MANT = 52;
};

/**
 * Generate a mask for bitwise operations.
 */
template <typename integer>
constexpr auto mask(int n) -> integer
{
	return ~(~static_cast<integer>(0) << n);
}

/**
 * Return an integer whose bit representation is the same as the given floating-
 * point number.
 */
template <typename real>
auto as_integral(real d) -> typename integral_repr<real>::type
{
	using integer = typename integral_repr<real>::type;

	static_assert(sizeof(integer) == sizeof(real), "mismatching memory size");

	integer b;
	unsigned char m[sizeof(real)];
	memcpy(m, &d, sizeof(real));
	memcpy(&b, m, sizeof(real));
	return b;
}

/**
 * Return a floating-point whose bit representation is the same as the given
 * integer number.
 */
template <typename integer>
auto as_floating_point(integer b) -> typename floating_repr<integer>::type
{
	using real = typename floating_repr<integer>::type;

	static_assert(sizeof(integer) == sizeof(real), "mismatching memory size");

	real d;
	unsigned char m[sizeof(real)];
	memcpy(m, &b, sizeof(real));
	memcpy(&d, m, sizeof(real));
	return d;
}

/**
 * Return true if the given floating-point number is negative.
 */
template <typename real>
bool is_negative(real d)
{
	/* most significant bit is set. */
	return (as_integral(d) < 0);
}

/**
 * Return true if the given floating-point number is a NaN.
 */
template <typename real>
bool is_nan(real d)
{
	bool a = std::isnan(d);
	bool b = (d != d);  /* The convention as defined by IEEE. */

	assert(a == b);

	return b;
}

/**
 * Return the exponent bits of the given floating-point number as an integer.
 */
template <typename real>
auto exponent(real a) -> typename integral_repr<real>::type
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	return (as_integral(a) >> traits::MANT) & mask<integer>(traits::EXP);
}

/**
 * Return the significand bits of the given floating-point number as an integer.
 */
template <typename real>
auto significand(real d) -> typename integral_repr<real>::type
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	return as_integral(d) & mask<integer>(traits::MANT);
}

/**
 * Return the header bits of the given floating-point number as an integer.
 */
template <typename real>
auto header(real d) -> typename integral_repr<real>::type
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	return (as_integral(d) >> traits::MANT) & mask<integer>(traits::HEADER);
}

/**
 * Return the payload of the given NaN as an integer.
 */
template <typename real>
auto nan_payload(real nan) -> typename integral_repr<real>::type
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	return as_integral(nan) & mask<integer>(traits::MANT - 1);
}

/**
 * Return true if the given NaN is quiet.
 */
template <typename real>
bool is_quiet(real nan)
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	return ((as_integral(nan) >> (traits::MANT - 1)) & mask<integer>(1)) != 0;
}

/**
 * Return true if the given NaN is positive.
 */
template <typename real>
bool is_positive_nan(real d)
{
	using traits = floating_point_info<real>;

	const auto a = is_nan(d) && !is_negative(d);
	const auto b = as_integral(d) > (traits::EXP_MASK << traits::MANT);
	const auto c = (header(d) == traits::EXP_MASK) && (significand(d) > 0);

	/* cppcheck-suppress compareBoolExpressionWithInt */
	assert((a == b) && (b == c));

	return b; /* Seems most efficient. */
}

/**
 * Return true if the given NaN is negative.
 */
template <typename real>
bool is_negative_nan(real d)
{
	using traits = floating_point_info<real>;

	bool a = is_nan(d) && is_negative(d);
	bool b = as_integral(d) < 0 && as_integral(d) > (traits::HEADER_MASK << traits::MANT);
	bool c = header(d) == traits::HEADER_MASK && significand(d) > 0;

	assert(a == b && b == c);

	return a; /* Seems most efficient. */
}

/**
 * Return a quiet NaN.
 */
template <typename real>
real make_quiet_nan(int payload)
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	integer b = (traits::EXP_MASK << traits::MANT) |
	            (integer(1) << (traits::MANT - 1)) |
	            (integer(payload) & mask<integer>(traits::MANT - 1));

	return as_floating_point(b);
}

/**
 * Return a signaling NaN.
 */
template <typename real>
real make_signaling_nan(int payload)
{
	using traits = floating_point_info<real>;
	using integer = typename traits::integer_t;

	assert((integer(payload) & mask<integer>(traits::MANT - 1)) != 0);

	auto b = (traits::EXP_MASK << traits::MANT) |
	         (integer(payload) & mask<integer>(traits::MANT - 1));

	/* NOTE: On x87, this signaling NaN will almost immediately become quiet. */
	return as_floating_point(b);
}

/**
 * Return an "infinite" number.
 */
template <typename real>
real make_inf()
{
	using traits = floating_point_info<real>;

	auto b = (traits::EXP_MASK << traits::MANT);
	return as_floating_point(b);
}

}  /* namespace nombre_decimaux */
}  /* namespace dls */
