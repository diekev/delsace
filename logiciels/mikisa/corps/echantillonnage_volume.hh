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

#include "volume.hh"

template <typename T>
auto reechantillonne(
		Grille<T> const &entree,
		float taille_voxel)
{
	auto desc = entree.desc();
	desc.taille_voxel = taille_voxel;

	auto resultat = Grille<T>(desc);
	auto res = resultat.resolution();
	auto res0 = entree.resolution();

	for (auto z = 0; z < res.z; ++z) {
		for (auto y = 0; y < res.y; ++y) {
			for (auto x = 0; x < res.x; ++x) {
				auto index = x + (y + z * res.y) * res.x;
				auto valeur = T(0);
				auto poids = 0.0f;

				auto pos_mnd = resultat.index_vers_monde(dls::math::vec3i(x, y, z));
				auto pos_orig = entree.monde_vers_index(pos_mnd);

				auto min_x = std::max(0, pos_orig.x - 1);
				auto min_y = std::max(0, pos_orig.y - 1);
				auto min_z = std::max(0, pos_orig.z - 1);

				auto max_x = std::min(res0.x, pos_orig.x + 1);
				auto max_y = std::min(res0.y, pos_orig.y + 1);
				auto max_z = std::min(res0.z, pos_orig.z + 1);

				for (auto zi = min_z; zi < max_z; ++zi) {
					for (auto yi = min_y; yi < max_y; ++yi) {
						for (auto xi = min_x; xi < max_x; ++xi) {
							auto index0 = xi + (yi + zi * res0.y) * res0.x;
							valeur += entree.valeur(index0);
							poids += 1.0f;
						}
					}
				}

				if (poids != 0.0f) {
					valeur /= poids;
				}

				resultat.valeur(index) = valeur;
			}
		}
	}

	return resultat;
}
