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

#include <cmath>

namespace dls::math {

template <typename T>
struct complexe {
private:
	T m_reel = 0;
	T m_imag = 0;

public:
	complexe() = default;

	complexe(T reel, T imag)
		: m_reel(reel)
		, m_imag(imag)
	{}

	T reel() const
	{
		return m_reel;
	}

	T &reel()
	{
		return m_reel;
	}

	T imag() const
	{
		return m_imag;
	}

	T &imag()
	{
		return m_imag;
	}

	complexe &operator+=(complexe const &autre)
	{
		m_reel += autre.m_reel;
		m_imag += autre.m_imag;
		return *this;
	}

	complexe &operator-=(complexe const &autre)
	{
		m_reel -= autre.m_reel;
		m_imag -= autre.m_imag;
		return *this;
	}

	complexe &operator*=(complexe const &autre)
	{
		auto tmp_r = m_reel * autre.m_reel - m_imag * autre.m_imag;
		auto tmp_i = m_reel * autre.m_imag + m_imag * autre.m_reel;
		m_reel = tmp_r;
		m_imag = tmp_i;
		return *this;
	}

	complexe &operator*=(T v)
	{
		m_reel *= v;
		m_imag *= v;
		return *this;
	}

	complexe &operator/=(T v)
	{
		m_reel /= v;
		m_imag /= v;
		return *this;
	}
};

template <typename T>
auto operator+(complexe<T> const &c1, complexe<T> const &c2)
{
	auto tmp(c1);
	tmp += c2;
	return tmp;
}

template <typename T>
auto operator-(complexe<T> const &c1, complexe<T> const &c2)
{
	auto tmp(c1);
	tmp -= c2;
	return tmp;
}

template <typename T>
auto operator*(complexe<T> const &c1, complexe<T> const &c2)
{
	auto tmp(c1);
	tmp *= c2;
	return tmp;
}

template <typename T>
auto operator*(complexe<T> const &c1, T v)
{
	auto tmp(c1);
	tmp *= v;
	return tmp;
}

template <typename T>
auto conjugue(complexe<T> const &c)
{
	return complexe<T>(c.reel(), -c.imag());
}

template <typename T>
auto exp(complexe<T> const &c)
{
	auto r = std::exp(c.reel());
	return complexe<T>(std::cos(c.imag()) * r, std::sin(c.imag()) * r);
}

template <typename T>
auto est_nan(complexe<T> const &c)
{
	return std::isnan(c.imag()) || std::isnan(c.imag());
}

template <typename T>
auto est_fini(complexe<T> const &c)
{
	return std::isfinite(c.imag()) && std::isfinite(c.imag());
}

template <typename T>
auto est_infini(complexe<T> const &c)
{
	return !est_fini(c);
}

}  /* namespace dls::math */
