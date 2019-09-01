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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "r16_c.h"
#pragma GCC diagnostic pop

/**
 * Type nombre réel sur 16-bits inspiré des types half de PTEX et OpenEXR.
 */

struct r16 {
	unsigned short bits = 0;

	inline r16() = default;

	inline r16(float f)
		: bits(DLS_depuis_r32(f))
	{}

	inline r16(double d)
		: bits(DLS_depuis_r64(d))
	{}

	inline r16 &operator+=(r16 f)
	{
		this->bits = DLS_ajoute_r16r16(this->bits, f.bits);
		return *this;
	}

	inline r16 &operator+=(float f)
	{
		this->bits = DLS_ajoute_r16r32(this->bits, f);
		return *this;
	}

	inline r16 &operator+=(double d)
	{
		this->bits = DLS_ajoute_r16r64(this->bits, d);
		return *this;
	}

	inline r16 &operator-=(r16 f)
	{
		this->bits = DLS_soustrait_r16r16(this->bits, f.bits);
		return *this;
	}

	inline r16 &operator-=(float f)
	{
		this->bits = DLS_soustrait_r16r32(this->bits, f);
		return *this;
	}

	inline r16 &operator-=(double d)
	{
		this->bits = DLS_soustrait_r16r64(this->bits, d);
		return *this;
	}

	inline r16 &operator*=(r16 f)
	{
		this->bits = DLS_multiplie_r16r16(this->bits, f.bits);
		return *this;
	}

	inline r16 &operator*=(float f)
	{
		this->bits = DLS_multiplie_r16r32(this->bits, f);
		return *this;
	}

	inline r16 &operator*=(double d)
	{
		this->bits = DLS_multiplie_r16r64(this->bits, d);
		return *this;
	}

	inline r16 &operator/=(r16 f)
	{
		this->bits = DLS_divise_r16r16(this->bits, f.bits);
		return *this;
	}

	inline r16 &operator/=(float f)
	{
		this->bits = DLS_divise_r16r32(this->bits, f);
		return *this;
	}

	inline r16 &operator/=(double d)
	{
		this->bits = DLS_divise_r16r64(this->bits, d);
		return *this;
	}

	inline operator float()
	{
		return DLS_vers_r32(this->bits);
	}

	inline operator double()
	{
		return DLS_vers_r64(this->bits);
	}
};

/* ******************************** additions ******************************* */

inline r16 operator+(r16 const &a, r16 const &b)
{
	auto tmp = a;
	tmp += b;
	return tmp;
}

inline r16 operator+(r16 const &a, float b)
{
	auto tmp = a;
	tmp += b;
	return tmp;
}

inline r16 operator+(float a, r16 const &b)
{
	auto tmp = b;
	tmp += a;
	return tmp;
}

inline r16 operator+(r16 const &a, double b)
{
	auto tmp = a;
	tmp += b;
	return tmp;
}

inline r16 operator+(double a, r16 const &b)
{
	auto tmp = b;
	tmp += a;
	return tmp;
}

/* ****************************** soustractions ***************************** */

inline r16 operator-(r16 const &a, r16 const &b)
{
	auto tmp = a;
	tmp -= b;
	return tmp;
}

inline r16 operator-(r16 const &a, float b)
{
	auto tmp = a;
	tmp -= b;
	return tmp;
}

inline r16 operator-(float a, r16 const &b)
{
	auto tmp = b;
	tmp -= a;
	return tmp;
}

inline r16 operator-(r16 const &a, double b)
{
	auto tmp = a;
	tmp -= b;
	return tmp;
}

inline r16 operator-(double a, r16 const &b)
{
	auto tmp = b;
	tmp -= a;
	return tmp;
}

/* ***************************** multiplications **************************** */

inline r16 operator*(r16 const &a, r16 const &b)
{
	auto tmp = a;
	tmp *= b;
	return tmp;
}

inline r16 operator*(r16 const &a, float b)
{
	auto tmp = a;
	tmp *= b;
	return tmp;
}

inline r16 operator*(float a, r16 const &b)
{
	auto tmp = b;
	tmp *= a;
	return tmp;
}

inline r16 operator*(r16 const &a, double b)
{
	auto tmp = a;
	tmp *= b;
	return tmp;
}

inline r16 operator*(double a, r16 const &b)
{
	auto tmp = b;
	tmp *= a;
	return tmp;
}

/* ******************************** divisions ******************************* */

inline r16 operator/(r16 const &a, r16 const &b)
{
	auto tmp = a;
	tmp /= b;
	return tmp;
}

inline r16 operator/(r16 const &a, float b)
{
	auto tmp = a;
	tmp /= b;
	return tmp;
}

inline r16 operator/(float a, r16 const &b)
{
	auto tmp = r16(a);
	tmp /= b;
	return tmp;
}

inline r16 operator/(r16 const &a, double b)
{
	auto tmp = a;
	tmp /= b;
	return tmp;
}

inline r16 operator/(double a, r16 const &b)
{
	auto tmp = r16(a);
	tmp /= b;
	return tmp;
}

/* ****************************** comparaisons ****************************** */

inline bool operator==(r16 const &a, r16 const &b)
{
	return DLS_compare_egl_r16r16(a.bits, b.bits);
}

inline bool operator==(r16 const &a, float b)
{
	return DLS_compare_egl_r16r32(a.bits, b);
}

inline bool operator==(float a, r16 const &b)
{
	return DLS_compare_egl_r32r16(a, b.bits);
}

inline bool operator==(r16 const &a, double b)
{
	return DLS_compare_egl_r16r64(a.bits, b);
}

inline bool operator==(double a, r16 const &b)
{
	return DLS_compare_egl_r64r16(a, b.bits);
}

inline bool operator!=(r16 const &a, r16 const &b)
{
	return !(a == b);
}

inline bool operator!=(r16 const &a, float b)
{
	return !(a == b);
}

inline bool operator!=(float a, r16 const &b)
{
	return !(a == b);
}

inline bool operator!=(r16 const &a, double b)
{
	return !(a == b);
}

inline bool operator!=(double a, r16 const &b)
{
	return !(a == b);
}

inline bool operator<(r16 const &a, r16 const &b)
{
	return DLS_compare_inf_r16r16(a.bits, b.bits);
}

inline bool operator<(r16 const &a, float b)
{
	return DLS_compare_inf_r16r32(a.bits, b);
}

inline bool operator<(float a, r16 const &b)
{
	return DLS_compare_inf_r32r16(a, b.bits);
}

inline bool operator<(r16 const &a, double b)
{
	return DLS_compare_inf_r16r64(a.bits, b);
}

inline bool operator<(double a, r16 const &b)
{
	return DLS_compare_inf_r64r16(a, b.bits);
}

inline bool operator<=(r16 const &a, r16 const &b)
{
	return (a == b) || (a < b);
}

inline bool operator<=(r16 const &a, float b)
{
	return (a == b) || (a < b);
}

inline bool operator<=(float a, r16 const &b)
{
	return (a == b) || (a < b);
}

inline bool operator<=(r16 const &a, double b)
{
	return (a == b) || (a < b);
}

inline bool operator<=(double a, r16 const &b)
{
	return (a == b) || (a < b);
}

inline bool operator>(r16 const &a, r16 const &b)
{
	return DLS_compare_sup_r16r16(a.bits, b.bits);
}

inline bool operator>(r16 const &a, float b)
{
	return DLS_compare_sup_r16r32(a.bits, b);
}

inline bool operator>(float a, r16 const &b)
{
	return DLS_compare_sup_r32r16(a, b.bits);
}

inline bool operator>(r16 const &a, double b)
{
	return DLS_compare_sup_r16r64(a.bits, b);
}

inline bool operator>(double a, r16 const &b)
{
	return DLS_compare_sup_r64r16(a, b.bits);
}

inline bool operator>=(r16 const &a, r16 const &b)
{
	return (a == b) || (a > b);
}

inline bool operator>=(r16 const &a, float b)
{
	return (a == b) || (a > b);
}

inline bool operator>=(float a, r16 const &b)
{
	return (a == b) || (a > b);
}

inline bool operator>=(r16 const &a, double b)
{
	return (a == b) || (a > b);
}

inline bool operator>=(double a, r16 const &b)
{
	return (a == b) || (a > b);
}

/* ********************************* autres ********************************* */

inline bool	est_finie(r16 const &a)
{
	return DLS_est_finie_r16(a.bits);
}

inline bool est_normalisee(r16 const &a)
{
	return DLS_est_normalisee_r16(a.bits);
}

inline bool est_denormalisee(r16 const &a)
{
	return DLS_est_denormalisee_r16(a.bits);
}

inline bool est_zero(r16 const &a)
{
	return DLS_est_zero_r16(a.bits);
}

inline bool est_nan(r16 const &a)
{
	return DLS_est_nan_r16(a.bits);
}

inline bool est_infinie(r16 const &a)
{
	return DLS_est_infinie_r16(a.bits);
}

inline bool est_negative(r16 const &a)
{
	return DLS_est_negative_r16(a.bits);
}

inline r16 r16_infinie_positive()
{
	auto tmp = r16();
	tmp.bits = DLS_r16_infinie_positive();
	return tmp;
}

inline r16 r16_infinie_negative()
{
	auto tmp = r16();
	tmp.bits = DLS_r16_infinie_negative();
	return tmp;
}

inline r16 r16_nan_q()
{
	auto tmp = r16();
	tmp.bits = DLS_r16_nan_q();
	return tmp;
}

inline r16 r16_nan_s()
{
	auto tmp = r16();
	tmp.bits = DLS_r16_nan_s();
	return tmp;
}
