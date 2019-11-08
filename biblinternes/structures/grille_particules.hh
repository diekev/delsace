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
#include "biblinternes/math/outils.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/tableau.hh"

class GrilleParticules {
	dls::math::point3d m_min{};
	dls::math::point3d m_max{};
	dls::math::point3d m_dim{};
	long m_res_x{};
	long m_res_y{};
	long m_res_z{};
	double m_distance{};

	dls::tableau<dls::tableau<dls::math::vec3f>> m_grille{};

public:
	explicit GrilleParticules(dls::math::point3d const &min, dls::math::point3d const &max, float distance);

	void ajoute(dls::math::vec3f const &position);

	bool verifie_distance_minimal(dls::math::vec3f const &point, float distance);

	bool triangle_couvert(
			dls::math::vec3f const &v0,
			dls::math::vec3f const &v1,
			dls::math::vec3f const &v2,
			const float radius);

	long calcul_index_pos(dls::math::vec3f const &point);
};

/* ************************************************************************** */

struct GrillePoint {
	struct DonneesPoint {
		dls::math::vec3f pos{};
		long idx{};

		DonneesPoint() = default;

		DonneesPoint(dls::math::vec3f const &p, long i)
			: pos(p)
			, idx(i)
		{}
	};

	using coord = dls::math::vec3i;
	using bloc  = limites3i;
	using tableau_index = dls::tableau<unsigned>;
	using tableau_point = dls::tableau<DonneesPoint>;

private:
	coord m_resolution{};
	dls::math::vec3f m_coord_min{};

	tableau_index m_index_cellules{};
	tableau_point m_points{};

	float m_taille_cellule{};

public:
	GrillePoint(float taille_cellule);

	static GrillePoint construit_avec_fonction(
			std::function<dls::math::vec3f(long)> points,
			long nombre_points,
			float taille_cellule);

	coord pos_grille(dls::math::vec3f const &p) const;

	long index_cellule(dls::math::vec3f const &p) const;

	long index_cellule(coord const &c) const;

	long nombre_elements() const;

	DonneesPoint *debut_points(long idx);

	DonneesPoint *fin_points(long idx);

	bloc cellules_autour(dls::math::vec3f const &p, float rayon);
};
