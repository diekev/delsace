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

#include <random>

#include "biblinternes/math/outils.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/empreintes.hh"

class GNA {
	std::mt19937 m_gna;

public:
	explicit GNA(unsigned long graine = 1);

	/* distribution uniforme */

	int uniforme(int min, int max);

	long uniforme(long min, long max);

	float uniforme(float min, float max);

	double uniforme(double min, double max);

	dls::math::vec3f uniforme_vec3(float min, float max);

	/* distribution normale */

	float normale(float moyenne, float ecart);

	double normale(double moyenne, double ecart);

	dls::math::vec3f normale_vec3(float moyenne, float ecart);
};

struct GNASimple {
private:
	unsigned m_graine = 0;

public:
	explicit GNASimple(unsigned graine)
		: m_graine(graine)
	{}

	/* distribution uniforme */

	inline float uniforme(float min, float max)
	{
		return (max - min) * empreinte_n32_vers_r32(m_graine++) + min;
	}

	inline double uniforme(double min, double max)
	{
		return (max - min) * static_cast<double>(empreinte_n32_vers_r32(m_graine++)) + min;
	}
};

template <typename TypeGNA>
[[nodiscard]] auto echantillone_disque_uniforme(TypeGNA &gna)
{
	auto x = 0.0f;
	auto y = 0.0f;
	auto l = 0.0f;

	do {
		x = gna.uniforme(-1.0f, 1.0f);
		y = gna.uniforme(-1.0f, 1.0f);
		l = std::sqrt(x * x + y * y);
	} while (l >= 1.0f || l == 0.0f);

	return dls::math::vec2f(x, y);
}

/* Retourne un nombre aléatoire avec une distribution normale (Gaussienne),
 * avec le sigma spécifié. Ceci utilise la forme polaire de Marsaglia de la
 * méthode de Box-Muller */
template <typename TypeGNA, typename T>
[[nodiscard]] auto echantillone_disque_normale(TypeGNA &gna, T moyenne = 0,  T sigma = 1)
{
	static constexpr auto _1 = static_cast<T>(1);
	static constexpr auto _2 = static_cast<T>(2);

	/* NOTE : pour éviter les problèmes numériques avec des nombres très petit,
	 * on pourrait utiliser des floats pour le calcul de u et v et des doubles
	 * pour le calcul de s.
	 */
	T u;
	T v;
	T s;

	do {
		u = gna.uniforme(-_1, _1);
		v = gna.uniforme(-_1, _1);
		s = u * u + v * v;
	} while (s >= _1 || dls::math::est_environ_zero(s));

	s = std::sqrt(-_2 * std::log(s) / s);
	auto tmp = s * sigma;

	return dls::math::vec2<T>(u * tmp + moyenne, v * tmp + moyenne);
}

template <typename type_vecteur, typename TypeGNA>
[[nodiscard]] auto echantillone_sphere(TypeGNA &gna) noexcept
{
	using type_scalaire = typename type_vecteur::type_scalaire;

	type_vecteur v;
	type_scalaire m2;

	do {
		m2 = static_cast<type_scalaire>(0);

		for (unsigned int i = 0; i < type_vecteur::nombre_composants; ++i) {
			v[i] = gna.uniforme(-static_cast<type_scalaire>(1), static_cast<type_scalaire>(1));
			m2 += dls::math::carre(v[i]);
		}
	} while (m2 > static_cast<type_scalaire>(1) || m2 == static_cast<type_scalaire>(0));

	return v / std::sqrt(m2);
}
