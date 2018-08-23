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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

template <typename T>
struct couleur {
	T r, v, b, a;

	explicit couleur(T valeur = 0.0f)
		: r(valeur)
		, v(valeur)
		, b(valeur)
		, a(valeur)
	{}

	couleur(T vr, T vv, T vb, T va)
		: r(vr)
		, v(vv)
		, b(vb)
		, a(va)
	{}

	const T operator[](int i) const
	{
		return *(&r + i);
	}

	T &operator[](int i)
	{
		return *(&r + i);
	}

	couleur &operator+=(const couleur &autre)
	{
		for (int i = 0; i < 4; ++i) {
			*(&r + i) += *(&autre.r + i);
		}

		return *this;
	}

	couleur &operator*=(const couleur &autre)
	{
		for (int i = 0; i < 4; ++i) {
			*(&r + i) *= *(&autre.r + i);
		}

		return *this;
	}

	couleur &operator*=(const T &valeur)
	{
		for (int i = 0; i < 4; ++i) {
			*(&r + i) *= valeur;
		}

		return *this;
	}

	couleur &operator/=(const couleur &autre)
	{
		for (int i = 0; i < 4; ++i) {
			*(&r + i) /= *(&autre.r + i);
		}

		return *this;
	}

	couleur &operator/=(const T &valeur)
	{
		for (int i = 0; i < 4; ++i) {
			*(&r + i) /= valeur;
		}

		return *this;
	}
};

using couleur8 = couleur<unsigned char>;
using couleur32 = couleur<float>;

template <typename T>
auto operator+(const couleur<T> &c, const couleur<T> &c2)
{
	auto tmp = c;
	tmp += c2;
	return tmp;
}

template <typename T>
auto operator*(const couleur<T> &c, const T valeur)
{
	auto tmp = c;
	tmp *= valeur;
	return tmp;
}

template <typename T>
auto operator*(const T valeur, const couleur<T> &c)
{
	auto tmp = c;
	tmp *= valeur;
	return tmp;
}

template <typename T>
auto operator/(const couleur<T> &c, const T valeur)
{
	auto tmp = c;
	tmp /= valeur;
	return tmp;
}

template <typename T>
auto operator/(const T valeur, const couleur<T> &c)
{
	auto tmp = couleur<T>(valeur);
	tmp /= c;
	return tmp;
}

template <typename T>
auto restreint(const couleur<T> &a, const T &min, const T &max)
{
	couleur<T> r;

	for (int i = 0; i < 4; ++i) {
		if (a[i] < min) {
			r[i] = min;
		}
		else if (a[i] > max) {
			r[i] = max;
		}
		else {
			r[i] = a[i];
		}
	}

	return r;
}

/**
 * Luminance calculée selon la recommandation ITU-R BT.709
 * https://en.wikipedia.org/wiki/Relative_luminance
 *
 * Les vraies valeurs sont :
 * ``Y = 0.2126390059(R) + 0.7151686788(V) + 0.0721923154(B)``
 * selon : "Derivation of Basic Television Color Equations", RP 177-1993
 *
 * Puisque ces valeurs ont une somme supérieur à 1.0, nous utilisons une des
 * valeurs tronquées pour que le résultat ne dépasse pas 1.0.
 */
template <typename T>
auto luminance(const couleur<T> &c)
{
	return c.r * 0.212671f + c.v * 0.715160f + c.b * 0.072169f;
}

/**
 * Calcul du contraste entre une couleur de zone (avant-plan) et une couleur
 * de fond (arrière-plan).
 *
 * https://fr.wikipedia.org/wiki/Contraste
 */
template <typename T>
auto calcul_contraste_local(const couleur<T> &zone, const couleur<T> &fond)
{
	auto luminosite_zone = luminance(zone);
	auto luminosite_fond = luminance(fond);

	if (luminosite_fond == 0.0f) {
		return 0.0f;
	}

	return (luminosite_zone - luminosite_fond) / luminosite_fond;
}

void rvb_vers_hsv(float r, float g, float b, float *lh, float *ls, float *lv);

void rvb_vers_hsv(const couleur32 &rvb, couleur32 &hsv);

void hsv_vers_rvb(float h, float s, float v, float *r, float *g, float *b);

void hsv_vers_rvb(const couleur32 &hsv, couleur32 &rvb);
