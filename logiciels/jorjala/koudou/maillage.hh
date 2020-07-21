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

#include <math.h>

#include "biblinternes/structures/tableau.hh"

#include "noeud.hh"
#include "tableau_index.hh"

namespace kdo {

struct maillage;

struct delegue_maillage {
	maillage const &ptr_maillage;

	delegue_maillage(maillage const &m);

	long nombre_elements() const;

	void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const;

	dls::phys::esectd intersecte_element(long idx, dls::phys::rayond const &rayon) const;
};

#define COMPRESSE_NORMAUX

#ifdef COMPRESSE_NORMAUX
// quantifie et déquantifie sur 15-bits (32768 = 2 ^ 15)
inline unsigned short quantifie(float f)
{
	return static_cast<unsigned short>(f * (32768.0f - 1.0f)) & 0x7fff;
}

inline auto dequantifie(unsigned short i)
{
	return static_cast<float>(i) * (1.0f / (32768.0f - 1.0f));
}

inline auto encode(dls::math::vec3f n)
{
	n /= (abs(n.x) + abs(n.y) + abs(n.z));

	auto ny = n.y * 0.5f + 0.5f;
	auto nx = n.x * 0.5f + ny;
	ny = n.x * -0.5f + ny;

	auto nz = dls::math::restreint(n.z * std::numeric_limits<float>::max(), 0.0f, 1.0f);
	return dls::math::vec3f(nx, ny, nz);
}

inline auto decode(dls::math::vec3f n)
{
	dls::math::vec3f resultat;
	resultat.x = (n.x - n.y);
	resultat.y = (n.x + n.y) - 1.0f;
	resultat.z = n.z * 2.0f - 1.0f;
	resultat.z = resultat.z * (1.0f - abs(resultat.x) - abs(resultat.y));
	return normalise(resultat);
}

inline auto quantifie_vec3(dls::math::vec3f const &v)
{
	auto qx = static_cast<unsigned int>(quantifie(v.x));
	auto qy = static_cast<unsigned int>(quantifie(v.y));
	auto qz = (v.z > 0.0f) ? 0u : 1u;

	return (qx << 17 | qy << 2 | qz);
}

inline auto dequantifie_vec3(unsigned int q)
{
	auto ix = (q >> 17) & 0x7fff;
	auto iy = (q >>  2) & 0x7fff;
	auto iz = q & 0x3;

	auto x = dequantifie(static_cast<unsigned short>(ix));
	auto y = dequantifie(static_cast<unsigned short>(iy));
	auto z = (iz) ? 0.0f : 1.0f;

	return dls::math::vec3f(x, y, z);
}

inline auto encode_et_quantifie(dls::math::vec3f const &v)
{
	auto r = encode(v);
	return quantifie_vec3(r);
}
#endif

struct maillage : public noeud {
	dls::tableau<dls::math::vec3f> points{};

#ifdef COMPRESSE_NORMAUX
	dls::tableau<uint> normaux{};
#else
	dls::tableau<dls::math::vec3f> normaux{};
#endif
	/* Nous gardons et des triangles et des quads pour économiser la mémoire.
	 * Puisqu'un quad = 2 triangles, pour chaque quad nous avons deux fois moins
	 * de noeuds dans l'arbre_hbe, ainsi que 1.5 fois moins d'index pour les
	 * points et normaux (2x3 pour les triangles vs 1x4 pour les quads).
	 */
	tableau_index triangles{};
	tableau_index quads{};

	tableau_index normaux_triangles{};
	tableau_index normaux_quads{};

	delegue_maillage delegue;

	int nombre_triangles = 0;
	int nombre_quads = 0;
	int index = 0;
	int volume = -1;

	maillage();

	void construit_arbre_hbe() override;

	dls::phys::esectd traverse_arbre(dls::phys::rayond const &rayon) override;

	limites3d calcule_limites() override;
};

}  /* namespace kdo */
