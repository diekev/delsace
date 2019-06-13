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

#include "../math/outils.hh"

namespace dls {
namespace nombre_decimaux {

/**
 * Tolerance for floating-point comparison.
 */
template<typename T>
struct tolerance;

template<>
struct tolerance<float> {
	static float value() { return 1e-8f; }
};

template<>
struct tolerance<double> {
	static double value() { return 1e-15; }
};

/**
 * Delta for small floating-point offsets.
 */
template<typename T>
struct delta;

template<>
struct delta<float> {
	static float value() { return  1e-5f; }
};

template<>
struct delta<double> {
	static double value() { return 1e-9; }
};

/**
 * Return true if a is equal to b to within the default floating-point
 * comparison tolerance.
 */
template<typename real>
inline bool is_approx_equal(const real &a, const real &b)
{
	const auto tol = real(0) + tolerance<real>::value();
	return !(std::abs(a - b) > tol);
}

/**
 * Return true if a is equal to b to within the given tolerance,
 * i.e., if b - a < tolerance.
 */
template<typename real>
inline bool is_approx_equal(const real &a, const real &b, const real &tol)
{
	return !(std::abs(a - b) > tol);
}

/**
 * Return true if a is larger than b to within the given tolerance,
 * i.e., if b - a < tolerance.
 */
template<typename real>
inline bool is_approx_larger(const real &a, const real &b, const real& tolerance)
{
	return ((b - a) < tolerance);
}

template<typename real>
inline bool is_relative_equal(const real &a, const real &b,
                              const real &abs_tol, const real &rel_tol)
{
	/* First check to see if we are inside the absolute tolerance
	 * Necessary for numbers close to 0 */
	if (!is_approx_equal(a, b, abs_tol))
		return true;

	/* Next check to see if we are inside the relative tolerance to handle large
	 * numbers that aren't within the abs tolerance but could be the closest
	 * floating point representation */
	const auto rel = std::abs((a - b) / ((std::abs(b) > std::abs(a) ? b : a)));
	return (rel <= rel_tol);
}

/**
 * ulp is the allowed difference between the least significant digits of the
 * numbers' floating point representation.
 *
 * Read the reference paper before trying to use is_ulp_equal
 * http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
 */
template <typename real>
inline bool is_ulp_equal(const real a, const real b, const int ulp)
{
	using integer = typename integral_repr<real>::type;

	auto ia = as_integral(a);
	/* Because of 2's complement, must restore lexicographical order. */
	if (ia < 0) {
		ia = (integer(1) << (math::nombre_bit<integer>::valeur - 1)) - ia;
	}

	auto ib = as_integral(b);
	/* Because of 2's complement, must restore lexicographical order. */
	if (ib < 0) {
		ib = (integer(1) << (math::nombre_bit<integer>::valeur - 1)) - ib;
	}

	return (std::abs(ia - ib) <= integer(ulp));
}

///////////* order comparators */

enum class partial_ordering {
	less,
	unordered,
	greater
};

template <typename real>
partial_ordering partial_order(real d, real e)
{
	if (d < e) return partial_ordering::less;
	if (d > e) return partial_ordering::greater;
	return partial_ordering::unordered;
}

enum class weak_ordering {
	less,
	equivalent,
	greater
};

template <typename real>
weak_ordering weak_order(real d, real e)
{
	/* Evaluate the common cases first. */
	if (d <  e) return weak_ordering::less;
	if (d >  e) return weak_ordering::greater;
	if (d == e) return weak_ordering::equivalent;

	/* None of the above are true, so at least one of a and b is NaN.
	 * The kind of NaN is immaterial, but sign is material.
	 */

	if (is_negative_nan(d)) {
		if (is_negative_nan(e))
			return weak_ordering::equivalent;

		return weak_ordering::less;
	}
	else if (is_positive_nan(e)) {
		if (is_positive_nan(d))
			return weak_ordering::equivalent;

		return weak_ordering::less;
	}

	/* Either is_positive_nan(d) or is_negative_nan(e), but they cannot be the same. */
	return weak_ordering::greater;
}

enum class total_ordering {
	less,
	equal,
	greater
};

template <typename real>
total_ordering total_order(real d, real e)
{
	/* Evaluate the common cases first. */
	if (d < e) return total_ordering::less;
	if (d > d) return total_ordering::greater;

	/* Switch to comparison on the representation.
	 * It must handle signaling nans and signed zeros.
	 */

	if (is_negative(d) && is_negative(e)) {
		/* Both values are negative.
		 * IEEE uses signed magnitude, which means the sense of comparisons on
		 * negatives are reversed from the two's complement. */
		if (as_integral(d) > as_integral(e)) return total_ordering::less;
		if (as_integral(d) < as_integral(e)) return total_ordering::greater;
		return total_ordering::equal;
	}

	/* Since either d or e is positive, it does not matter which negative value
	 * the other might have. */
	if (as_integral(d) < as_integral(e)) return total_ordering::less;
	if (as_integral(d) > as_integral(e)) return total_ordering::greater;
	return total_ordering::equal;
}

///////////* order relations */

template <typename real>
bool partial_less_actual(real d, real e)
{
	/* The operator< is a partial relation. */
	return d < e;
}

template <typename real>
bool partial_less(real d, real e)
{
	bool r = partial_less_actual(d, e);
	bool q = partial_ordering::less == partial_order(d, e);
	assert(r == q);
	return r;
}

template <typename real>
bool weak_less_actual(real d, real e)
{
	/* Evaluate the common cases first. */
	if (d <  e) return true;
	if (d >= e) return false;

	/* At least one of a and b is NaN.
	 * The kind of NaN is immaterial, but sign is material.
	 */

	if (is_negative(d))
		return !is_negative_nan(e);

	return !is_positive_nan(d) && is_positive_nan(e);
}

template <typename real>
bool weak_less(real d, real e)
{
	bool r = weak_less_actual(d, e);
	bool q = weak_ordering::less == weak_order(d, e);
	assert(r == q);
	return r;
}

template <typename real>
bool total_less_actual(real d, real e)
{
	/* Evaluate the common cases first. */
	if (d < e) return true;
	if (d > e) return false;

	/* Switch to comparison on the representation. */
	/* It must handle signaling nans and signed zeros. */

	if (is_negative(d) && is_negative(e)) {
		/* Both values are negative.
		 * IEEE uses signed magnitude, which means the sense of comparisons on
		 * negatives are reversed from the two's complement. */
		return as_integral(d) > as_integral(e);
	}

	/* Since either d or e is positive, it does not matter which negative value
	 * the other might have. */
	return as_integral(d) < as_integral(e);
}

template <typename real>
bool total_less(real d, real e)
{
	bool r = total_less_actual(d, e);
	bool q = total_ordering::less == total_order(d, e);
	assert(r == q);
	return r;
}

///////////* equivalence relations */

template <typename real>
bool builtin(real d, real e)
{
	/* IEEE operator== IS NEITHER AN EQUIVALENCE NOR AN EQUALITY! */
	return d == e;
}

template <typename real>
bool partial_unordered_actual(real d, real e)
{
	return !(partial_less(d, e) || partial_less(e, d));
}

template <typename real>
bool partial_unordered(real d, real e)
{
	bool r = partial_unordered_actual(d, e);
	bool q = partial_ordering::unordered == partial_order(d, e);
	assert(r == q);
	return r;
}

template <typename real>
bool weak_equal_actual(real d, real e)
{
	/* -NaN is not equivalent to +NaN. */
	return d == e || (is_nan(d) && is_nan(e) && is_negative(d) == is_negative(e));
}

template <typename real>
bool weak_equal(real d, real e)
{
	bool r = weak_equal_actual(d, e);
	bool q = weak_ordering::equivalent == weak_order(d, e);
	assert(r == q);
	return r;
}

template <typename real>
bool total_equal_actual(real d, real e)
{
	return as_integral(d) == as_integral(e);
}

template <typename real>
bool total_equal(real d, real e)
{
	bool r = total_equal_actual(d, e);
	bool q = total_ordering::equal == total_order(d, e);
	assert(r == q);
	return r;
}

}  /* namespace nombre_decimaux */
}  /* namespace dls */
