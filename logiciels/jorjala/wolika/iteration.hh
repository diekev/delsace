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

template <typename type_vec>
struct IteratriceCoordonnees {
private:
	using TypeVecteur = type_vec;
	static constexpr auto DIMS = TypeVecteur::nombre_composants;

	limites<TypeVecteur> m_lim{};
	TypeVecteur m_etat;

public:
	IteratriceCoordonnees(limites<TypeVecteur> const &lim)
		: m_lim(lim)
		, m_etat(lim.min)
	{}

	TypeVecteur suivante()
	{
		auto etat = m_etat;

		m_etat[0] += 1;

		for (auto i = 0u; i < DIMS - 1; ++i) {
			if (m_etat[i] < m_lim.max[i + 1]) {
				break;
			}

			m_etat[i] = m_lim.min[i];
			m_etat[i + 1] += 1;
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat[DIMS - 1] >= m_lim.max[DIMS - 1];
	}
};

template <typename type_vec>
struct IteratriceCoordonneesInv {
private:
	using TypeVecteur = type_vec;
	static constexpr auto DIMS = TypeVecteur::nombre_composants;

	limites<TypeVecteur> m_lim{};
	TypeVecteur m_etat;

public:
	IteratriceCoordonneesInv(limites<TypeVecteur> const &lim)
		: m_lim(lim)
		, m_etat(lim.max)
	{}

	TypeVecteur suivante()
	{
		auto etat = m_etat;

		m_etat[0] -= 1;

		for (auto i = 0u; i < DIMS - 1; ++i) {
			if (m_etat[i] >= m_lim.min[i + 1]) {
				break;
			}

			m_etat[i] = m_lim.max[i] - 1;
			m_etat[i + 1] -= 1;
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat[DIMS - 1] < m_lim.min[DIMS - 1];
	}
};

using IteratricePosition = IteratriceCoordonnees<dls::math::vec3i>;
using IteratricePositionInv = IteratriceCoordonneesInv<dls::math::vec3i>;

#if 0
template <template <typename> typename type_vec>
struct IndexeuseCoordonnees {
	using TypeVecteur = type_vec<int>;
	static constexpr auto DIMS = TypeVecteur::DIMS;

	int res[DIMS];
	int poids[DIMS];

	IndexeuseCoordonnees(TypeVecteur r)
	{
		for (auto i = 0u; i < DIMS; ++i) {
			res[i] = r[i];
			poids[i] = 1;
		}

		for (auto i = 1u; i < DIMS; ++i) {
			for (auto j = 0u; j < i; ++j) {
				poids[i] *= res[j];
			}
		}
	}

	long index(TypeVecteur const &co)
	{
		long idx = co[0];

		for (auto i = 1u; i < DIMS; ++i) {
			idx += co[i] * poids[i];
		}

		return idx;
	}
};
#endif

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

/**
 * Transforme une grille en utilisant une grille temporaire pour ne pas polluer
 * les données. L'opérateur doit être de la forme T(grille_temp, x, y, z).
 */
template <typename T, typename Op>
auto transforme_grille(
		grille_dense_3d<T> &grille,
		Op &&op,
		interruptrice *chef = nullptr)
{
	auto temp = grille;

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
					grille.valeur(idx) = op(temp, x, y, z);
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
