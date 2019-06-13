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

#include <iostream>

namespace dls {
namespace types {

/**
 * Convert from degree celsius to fahrenheit.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ctof(const T t) { return (T(5) * (t - T(32))) / T(9); }

/**
 * Convert from degree celsius to kelvin.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ctok(const T t) { return t + T(273.15); }

/**
 * Convert from degree celsius to rankine.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ctor(const T t) { return T(1.8) * t + T(491.67); }

/**
 * Convert from degree fahrenheit to celsius.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ftoc(const T t) { return (t * T(9)) / T(5) + T(32); }

/**
 * Convert from degree fahrenheit to kelvin.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ftok(const T t) { return (t + T(459.67)) / T(1.8); }

/**
 * Convert from degree fahrenheit to rankine.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ftor(const T t) { return t + T(459.67); }

/**
 * Convert from degree kelvin to celsius.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ktoc(const T t) { return t - T(273.15); }

/**
 * Convert from degree kelvin to fahrenheit.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ktof(const T t) { return T(1.8) * t - T(459.67); }

/**
 * Convert from degree kelvin to rankine.
 * @param t The temperature to convert.
 */
template <typename T>
inline T ktor(const T t) { return t * T(1.8); }

/**
 * Convert from degree rankine to celsius.
 * @param t The temperature to convert.
 */
template <typename T>
inline T rtoc(const T t) { return (t - T(491.67)) / T(1.8); }

/**
 * Convert from degree rankine to fahrenheit.
 * @param t The temperature to convert.
 */
template <typename T>
inline T rtof(const T t) { return t - T(459.67); }

/**
 * Convert from degree rankine to kelvin.
 * @param t The temperature to convert.
 */
template <typename T>
inline T rtok(const T t) { return t / T(1.8); }

/* convert from one temparature scale to the same scale */
template <typename T>
inline T noop(const T t) { return t; }

enum TemperatureScale {
	CELSIUS     = 0,
	FAHRENHEIT  = 1,
	KELVIN      = 2,
	RANKINE     = 3,
};

template <typename T>
class temperature {
	T m_theta;
	TemperatureScale m_scale;

	typedef T (*conversion_func_t)(const T);

	const conversion_func_t conversion_func[4][4] = {
	    { noop, ctof, ctok, ctor },
	    { ftoc, noop, ftok, ftor },
	    { ktoc, ktof, noop, ktor },
	    { rtoc, rtof, rtok, noop }
	};

public:
	using value_type = T;

	temperature() = delete;
	~temperature() noexcept = default;

	temperature(const temperature &other) noexcept
		: m_theta(other.m_theta)
		, m_scale(other.m_scale)
	{}

	temperature(const T theta, const TemperatureScale &scale) noexcept
		: m_theta(theta)
		, m_scale(scale)
	{}

	/* ============================= main methods ============================ */

	temperature &to_celsius() noexcept
	{
		scale(CELSIUS);
		return *this;
	}

	temperature &to_fahrenheit() noexcept
	{
		scale(FAHRENHEIT);
		return *this;
	}

	temperature &to_kelvin() noexcept
	{
		scale(KELVIN);
		return *this;
	}

	temperature &to_rankine() noexcept
	{
		scale(RANKINE);
		return *this;
	}

	void scale(const TemperatureScale &scale, const bool convert = true) noexcept
	{
		if (convert) {
			m_theta = conversion_func[m_scale][scale](m_theta);
		}

		m_scale = scale;
	}

	const TemperatureScale &scale() const noexcept
	{
		return m_scale;
	}

	const T value() const noexcept
	{
		return m_theta;
	}

	const T valueAs(const TemperatureScale &scale) const noexcept
	{
		return conversion_func[this->m_scale][scale](m_theta);
	}

	/* ============================== operators ============================== */

	temperature &operator=(const temperature &other) noexcept
	{
		this->m_theta = other.m_theta;
		this->m_scale = other.m_scale;
		return *this;
	}

	const temperature &operator+=(const temperature &other) noexcept
	{
		this->m_theta += conversion_func[this->m_scale][other.m_scale](other.m_theta);
		return *this;
	}

	template <typename S>
	const temperature &operator+=(const S t) noexcept
	{
		this->m_theta += t;
		return *this;
	}

	const temperature &operator-=(const temperature &other) noexcept
	{
		this->m_theta -= conversion_func[this->m_scale][other.m_scale](other.m_theta);
		return *this;
	}

	template <typename S>
	const temperature &operator-=(const S t) noexcept
	{
		this->m_theta -= t;
		return *this;
	}

	const temperature &operator/=(const temperature &other) noexcept
	{
		T t = conversion_func[this->m_scale][other.m_scale](other.m_theta);
		this->m_theta = (t == T(0)) ? T(0) : this->m_theta / t;
		return *this;
	}

	template <typename S>
	const temperature &operator/=(const S t) noexcept
	{
		this->m_theta = (t == S(0)) ? S(0) : this->m_theta / t;
		return *this;
	}

	const temperature &operator*=(const temperature &other) noexcept
	{
		this->m_theta *= conversion_func[this->m_scale][other.m_scale](other.m_theta);
		return *this;
	}

	template <typename S>
	const temperature &operator*=(const S t) noexcept
	{
		this->m_theta *= t;
		return *this;
	}

	temperature operator-() const noexcept
	{
		return {
			-this->m_theta,
			 this->m_scale
		};
	}
};

template <typename T>
inline temperature<T> operator+(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	temperature<T> result(th1);
	result += th2;
	return result;
}

template <typename T, typename S>
inline temperature<T> operator+(const temperature<T> &th1, const S t) noexcept
{
	temperature<T> result(th1);
	result += t;
	return result;
}

template <typename T>
inline temperature<T> operator-(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	temperature<T> result(th1);
	result -= th2;
	return result;
}

template <typename T, typename S>
inline temperature<T> operator-(const temperature<T> &th1, const S t) noexcept
{
	temperature<T> result(th1);
	result -= t;
	return result;
}

template <typename T>
inline temperature<T> operator/(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	temperature<T> result(th1);
	result /= th2;
	return result;
}

template <typename T, typename S>
inline temperature<T> operator/(const temperature<T> &th1, const S t) noexcept
{
	temperature<T> result(th1);
	result /= t;
	return result;
}

template <typename T>
inline temperature<T> operator*(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	temperature<T> result(th1);
	result *= th2;
	return result;
}

template <typename T, typename S>
inline temperature<T> operator*(const temperature<T> &th1, const S t) noexcept
{
	temperature<T> result(th1);
	result *= t;
	return result;
}

template <typename T>
bool operator==(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return (th1.scale() == th2.scale()) && (th1.value() == th2.value());
}

template <typename T>
bool operator!=(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return !(th1 == th2);
}

template <typename T>
bool operator<(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return th1.value() < th2.valueAs(th1.scale());
}

template <typename T>
bool operator<=(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return (th1 < th2) || (th1 == th2);
}

template <typename T>
bool operator>(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return th1.value() > th2.valueAs(th1.scale());
}

template <typename T>
bool operator>=(const temperature<T> &th1, const temperature<T> &th2) noexcept
{
	return (th1 > th2) || (th1 == th2);
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const temperature<T> &t) noexcept(false)
{
	const char *suffix[4] = { " °C", " °F", " °K", " °R" };
	os << t.value() << suffix[t.scale()];
	return os;
}

}  /* namespace math */
}  /* namespace dls */
