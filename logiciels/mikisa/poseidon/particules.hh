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

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/tableau.hh"

#include "corps/volume.hh"

namespace psn {

struct Particule {
	dls::math::vec3f pos{};
	float densite{};
};

struct GrilleParticule {
private:
	Grille<int> m_grille{};
	dls::tableau< dls::tableau<Particule *>> m_cellules{};

public:
	GrilleParticule() = default;

	GrilleParticule(description_volume const &desc)
		: m_grille(desc, -1)
	{}

	dls::tableau<Particule *> voisines_cellules(
			const dls::math::vec3i& index,
			const dls::math::vec3i& numberOfNeighbors)
	{
		//loop through neighbors, for each neighbor, check if cell has particles and push back contents
		dls::tableau<Particule *> neighbors;

		auto ix = index.x;
		auto iy = index.y;
		auto iz = index.z;

		auto nvx = numberOfNeighbors.x;
		auto nvy = numberOfNeighbors.y;
		auto nvz = numberOfNeighbors.z;

		auto res = m_grille.resolution();

		for (auto sx = ix - numberOfNeighbors.x; sx <= ix + nvx; sx += 1) {
			for (auto sy = iy - numberOfNeighbors.y; sy <= iy + nvy; sy += 1) {
				for (auto sz = iz - numberOfNeighbors.z; sz <= iz + nvz; sz += 1) {
					if ( sx < 0 || sx > res.x-1
						 || sy < 0 || sy > res.y-1
						 || sz < 0 || sz > res.z-1 ) {
						continue;
					}

					auto cellindex = m_grille.valeur(sx, sy, sz);

					if (cellindex != -1l) {
						auto cellparticlecount = m_cellules[cellindex].taille();

						for (auto a = 0l; a < cellparticlecount; a++) {
							neighbors.pousse(m_cellules[cellindex][a]);
						}
					}
				}
			}
		}

		return neighbors;
	}

	void tri(dls::tableau<Particule *> const &particles)
	{
		for (auto &cellule : m_cellules) {
			cellule.efface();
		}

		for (auto p : particles) {
			auto pos_cellule = m_grille.monde_vers_index(p->pos);
			auto idx_cellule = m_grille.valeur(pos_cellule.x, pos_cellule.y, pos_cellule.z);

			if (idx_cellule >= 0) {
				m_cellules[idx_cellule].pousse(p);
			}
			else {
				dls::tableau<Particule *> cellule;
				cellule.pousse(p);
				m_grille.valeur(pos_cellule.x, pos_cellule.y, pos_cellule.z) = static_cast<int>(m_cellules.taille());
				m_cellules.pousse(cellule);
			}
		}
	}
};

}  /* namespace psn */
