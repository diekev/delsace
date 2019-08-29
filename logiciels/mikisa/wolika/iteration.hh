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
#include "biblinternes/moultfilage/boucle.hh"

#include "grille_dense.hh"
#include "grille_eparse.hh"
#include "interruptrice.hh"

namespace wlk {

class IteratricePosition {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePosition(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.min)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x += 1;

		if (m_etat.x >= m_lim.max.x) {
			m_etat.x = m_lim.min.x;

			m_etat.y += 1;

			if (m_etat.y >= m_lim.max.y) {
				m_etat.y = m_lim.min.y;

				m_etat.z += 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z >= m_lim.max.z;
	}
};

class IteratricePositionInv {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePositionInv(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.max)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x -= 1;

		if (m_etat.x < m_lim.min.x) {
			m_etat.x = m_lim.max.x - 1;

			m_etat.y -= 1;

			if (m_etat.y < m_lim.min.y) {
				m_etat.y = m_lim.max.y - 1;

				m_etat.z -= 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z < m_lim.min.z;
	}
};

template <typename T, typename Op>
auto pour_chaque_voxel_parallele(
		grille_dense_3d<T> &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto res_x = grille.desc().resolution.x;
	auto res_y = grille.desc().resolution.y;
	auto res_z = grille.desc().resolution.z;

	boucle_parallele(tbb::blocked_range<int>(0, res_z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef && chef->interrompue()) {
			return;
		}

		for (auto z = plage.begin(); z < plage.end(); ++z) {
			if (chef && chef->interrompue()) {
				return;
			}

			for (auto y = 0; y < res_y; ++y) {
				if (chef && chef->interrompue()) {
					return;
				}

				for (auto x = 0; x < res_x; ++x) {
					auto idx = grille.calcul_index(dls::math::vec3i(x, y, z));

					grille.valeur(idx) = op(grille.valeur(idx), idx, x, y, z);
				}
			}

			if (chef) {
				auto delta = static_cast<float>(plage.end() - plage.begin());
				delta /= static_cast<float>(res_z);
				chef->indique_progression_parallele(delta * 100.0f);
			}
		}
	});
}

template <typename T, typename type_tuile, typename Op>
auto pour_chaque_tuile(
		grille_eparse<T, type_tuile> const &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto plg = grille.plage();
	auto idx = 0;
	auto nombre_tuiles = grille.nombre_tuile();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		if (chef && chef->interrompue()) {
			return;
		}

		op(tuile);

		if (chef) {
			auto delta = static_cast<float>(idx++);
			delta /= static_cast<float>(nombre_tuiles);
			chef->indique_progression_parallele(delta * 100.0f);
		}
	}
}

template <typename T, typename type_tuile, typename Op>
auto pour_chaque_tuile(
		grille_eparse<T, type_tuile> &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto plg = grille.plage();
	auto idx = 0;
	auto nombre_tuiles = grille.nombre_tuile();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		if (chef && chef->interrompue()) {
			return;
		}

		op(tuile);

		if (chef) {
			auto delta = static_cast<float>(idx++);
			delta /= static_cast<float>(nombre_tuiles);
			chef->indique_progression_parallele(delta * 100.0f);
		}
	}
}

template <typename T, typename type_tuile, typename Op>
auto pour_chaque_tuile_parallele(
		grille_eparse<T, type_tuile> const &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto nombre_tuiles = grille.nombre_tuile();

	tbb::parallel_for(0l, nombre_tuiles, [&](long i)
	{
		if (chef && chef->interrompue()) {
			return;
		}

		op(grille.tuile(i));

		if (chef) {
			auto delta = static_cast<float>(i) / static_cast<float>(nombre_tuiles);
			chef->indique_progression_parallele(delta * 100.0f);
		}
	});
}

template <typename T, typename type_tuile, typename Op>
auto pour_chaque_tuile_parallele(
		grille_eparse<T, type_tuile> &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto nombre_tuiles = grille.nombre_tuile();

	tbb::parallel_for(0l, nombre_tuiles, [&](long i)
	{
		if (chef && chef->interrompue()) {
			return;
		}

		op(grille.tuile(i));

		if (chef) {
			auto delta = static_cast<float>(i) / static_cast<float>(nombre_tuiles);
			chef->indique_progression_parallele(delta * 100.0f);
		}
	});
}

}  /* namespace wlk */
