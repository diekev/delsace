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

#include "biblinternes/math/limites.hh"

/* ************************************************************************** */

/* idée pour un typage strict
using co_monde = dls::math::vec3f;
using co_locale = dls::math::vecteur<10, float, 0, 1, 2>;    // (0.0 : 1.0)
using co_continue = dls::math::vecteur<11, float, 0, 1, 2>; // (0.0 : res.0)
using co_discrete = dls::math::vec3i;    // (0 : res - 1)
*/

/* ************************************************************************** */

/**
 * Coordonnées :
 * - mondiales
 * - locales (entre 0 et 1)
 * - élémentaires (entre 0 et res - 1)
 */

/* ************************************************************************** */


namespace wlk {

enum class type_grille : int {
	N32,
	Z8,
	Z32,
	R32,
	R32_PTR, /* pour les calques profond */
	R64,
	VEC2,
	VEC3,
};

template <typename T>
struct selectrice_type_grille;

template <> struct selectrice_type_grille<unsigned int>     { static constexpr auto type = type_grille::N32; };
template <> struct selectrice_type_grille<char>             { static constexpr auto type = type_grille::Z8; };
template <> struct selectrice_type_grille<int>              { static constexpr auto type = type_grille::Z32; };
template <> struct selectrice_type_grille<float>            { static constexpr auto type = type_grille::R32; };
template <> struct selectrice_type_grille<float *>          { static constexpr auto type = type_grille::R32_PTR; };
template <> struct selectrice_type_grille<double>           { static constexpr auto type = type_grille::R64; };
template <> struct selectrice_type_grille<dls::math::vec2f> { static constexpr auto type = type_grille::VEC2; };
template <> struct selectrice_type_grille<dls::math::vec3f> { static constexpr auto type = type_grille::VEC3; };

/* ************************************************************************** */

/**
 * Une grille peut avoir plusieurs limites : les limites de son tampon de
 * données (pixels, voxels), ou les limites de sa fenêtre de données.
 */
template <template<typename> typename type_vec>
struct description_grille {
	limites<type_vec<float>> etendue{};
	limites<type_vec<float>> fenetre_donnees{};
	type_vec<int> resolution{};

	union {
		double taille_pixel = 0.0;
		double taille_voxel;
	};

	type_grille type_donnees{};
	char pad[4];
};

template <template<typename> typename type_vec>
struct base_grille {
	using type_desc = description_grille<type_vec>;

protected:
	description_grille<type_vec> m_desc{};
	long m_nombre_elements = 0;

	bool hors_des_limites(type_vec<int> const &co) const
	{
		return dls::math::hors_limites(co, type_vec<int>(0), m_desc.resolution - type_vec<int>(1));
	}

public:
	base_grille() = default;

	base_grille(type_desc const &descr)
		: m_desc(descr)
	{
		auto taille = dls::math::converti_type<double>(m_desc.etendue.taille());
		taille /= m_desc.taille_pixel;

		m_desc.resolution = dls::math::converti_type<int>(taille);

		auto res_long = dls::math::converti_type<long>(m_desc.resolution);
		m_nombre_elements = dls::math::produit_interne(res_long);
	}

	virtual ~base_grille() = default;

	/* entreface */

	virtual base_grille *copie() const = 0;

	virtual bool est_eparse() const
	{
		return false;
	}

	/* accès propriétés */

	type_desc const &desc() const
	{
		return m_desc;
	}

	long nombre_elements() const
	{
		return m_nombre_elements;
	}

	long calcul_index(type_vec<int> const &co) const
	{
		return dls::math::calcul_index(co, m_desc.resolution);
	}

	/* conversion coordonnées */

	/**
	 * L'espace index (voxel/pixel) peut être accéder de deux façons : par des
	 * coordonnées entières ou par des coordonnées décimales. L'accès entier est
	 * utilisé pour des accès directs à un élément individuel, et l'accès
	 * décimal pour les entrepolations. Il faut faire attention en convertissant
	 * entre les deux. Le centre de l'élément (0, 0, 0) a les coordonnées
	 * (0.5, 0.5, 0.5). Ainsi, les côtés d'une grille avec une résolution de 100
	 * sont à 0.0 et 100.0 quand on utilise des coordonnées décimales, mais de 0
	 * et 99 pour des entières. Un bon survol de ceci peut se trouver dans un
	 * article de Paul S. Heckbert :
	 * What Are The Coordinates Of A Pixel? [Heckbert, 1990]
	 */

	/* converti un point de l'espace voxel discret vers l'espace unitaire */
	type_vec<float> index_vers_unit(const type_vec<int> &vsp) const
	{
		auto p = dls::math::discret_vers_continu<float>(vsp);
		return index_vers_unit(p);
	}

	/* converti un point de l'espace voxel continu vers l'espace unitaire */
	type_vec<float> index_vers_unit(const type_vec<float> &vsp) const
	{
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return vsp / resf;
	}

	/* converti un point de l'espace voxel continu vers l'espace unitaire */
	type_vec<float> index_vers_monde(const type_vec<int> &isp) const
	{
		auto const dim = m_desc.etendue.taille();
		auto const min = m_desc.etendue.min;
		auto const cont = dls::math::discret_vers_continu<float>(isp);
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return cont / resf * dim + min;
	}

	/* converti un point de l'espace unitaire (0.0 - 1.0) vers l'espace mondial */
	type_vec<float> unit_vers_monde(const type_vec<float> &vsp) const
	{
		return vsp * m_desc.etendue.taille() + m_desc.etendue.min;
	}

	/* converti un point de l'espace mondial vers l'espace unitaire (0.0 - 1.0) */
	type_vec<float> monde_vers_unit(const type_vec<float> &wsp) const
	{
		return (wsp - m_desc.etendue.min) / m_desc.etendue.taille();
	}

	/* converti un point de l'espace mondial vers l'espace continu */
	type_vec<float> monde_vers_continu(const type_vec<float> &wsp) const
	{
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return monde_vers_unit(wsp) * resf;
	}

	/* converti un point de l'espace mondial vers l'espace index */
	type_vec<int> monde_vers_index(const type_vec<float> &wsp) const
	{
		auto idx = monde_vers_continu(wsp);
		return dls::math::converti_type<int>(idx);
	}

	/* converti un point de l'espace continu vers l'espace mondial */
	type_vec<float> continu_vers_monde(const type_vec<float> &csp) const
	{
		return (csp / m_desc.etendue.taille()) + m_desc.etendue.min;
	}
};

using desc_grille_2d = description_grille<dls::math::vec2>;
using base_grille_2d = base_grille<dls::math::vec2>;

void deloge_grille(base_grille_2d *&tampon);

using desc_grille_3d = description_grille<dls::math::vec3>;
using base_grille_3d = base_grille<dls::math::vec3>;

}  /* namespace wlk */
