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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tableau.hh"

#include "base_grille.hh"

namespace wlk {

/* ************************************************************************** */

static constexpr auto TAILLE_TUILE = 8;
static constexpr auto VOXELS_TUILE = TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE;

template <typename T>
struct tuile_scalaire {
	using type_valeur = T;

	type_valeur donnees[static_cast<size_t>(VOXELS_TUILE)];
	dls::math::vec3i min{};
	dls::math::vec3i max{};
	bool garde = false;
	bool visite = false;

	static void detruit(tuile_scalaire *&t)
	{
		memoire::deloge("tuile", t);
	}

	static type_valeur echantillonne(tuile_scalaire *t, long index, float temps)
	{
		INUTILISE(temps);
		return t->donnees[static_cast<size_t>(index)];
	}

	static void copie_donnees(tuile_scalaire const *de, tuile_scalaire *vers)
	{
		for (auto j = 0; j < VOXELS_TUILE; ++j) {
			vers->donnees[j] = de->donnees[j];
		}
	}
};

template <typename T, typename TypeTuile = tuile_scalaire<T>>
struct grille_eparse : public base_grille_3d {
	using type_valeur = T;
	using type_topologie = dls::tableau<long>;
	using type_tuile = TypeTuile;

private:
	type_topologie m_index_tuiles{};
	dls::tableau<type_tuile *> m_tuiles{};

	int m_tuiles_x = 0;
	int m_tuiles_y = 0;
	int m_tuiles_z = 0;

	type_valeur m_arriere_plan = type_valeur(0);

	bool hors_des_limites(int i, int j, int k) const
	{
		if (i < 0 || i >= m_tuiles_x) {
			return true;
		}

		if (j < 0 || j >= m_tuiles_y) {
			return true;
		}

		if (k < 0 || k >= m_tuiles_z) {
			return true;
		}

		return false;
	}

	inline long index_tuile(long i, long j, long k) const
	{
		return i + (j + k * m_tuiles_y) * m_tuiles_x;
	}

	inline int converti_nombre_tuile(int i) const
	{
		return i / TAILLE_TUILE + ((i % TAILLE_TUILE) != 0);
	}

public:
	using plage_tuile = dls::plage_continue<type_tuile *>;
	using plage_tuile_const = dls::plage_continue<type_tuile * const>;

	grille_eparse(desc_grille_3d const &descr)
		: base_grille_3d(descr)
	{
		this->m_desc.type_donnees = selectrice_type_grille<T>::type;

		m_tuiles_x = converti_nombre_tuile(m_desc.resolution.x);
		m_tuiles_y = converti_nombre_tuile(m_desc.resolution.y);
		m_tuiles_z = converti_nombre_tuile(m_desc.resolution.z);

		auto nombre_tuiles = m_tuiles_x * m_tuiles_y * m_tuiles_z;

		m_index_tuiles.redimensionne(nombre_tuiles, -1l);
	}

	grille_eparse(grille_eparse const &) = default;
	grille_eparse &operator=(grille_eparse const &) = default;

	~grille_eparse()
	{
		for (auto t : m_tuiles) {
			type_tuile::detruit(t);
		}
	}

	void assure_tuiles(limites3f const &fenetre_donnees)
	{
		auto min = monde_vers_index(fenetre_donnees.min);
		auto max = monde_vers_index(fenetre_donnees.max);

		assure_tuiles(limites3i{min, max});
	}

	void assure_tuiles(limites3i const &fenetre_donnees)
	{
		auto min_tx = fenetre_donnees.min.x / TAILLE_TUILE;
		auto min_ty = fenetre_donnees.min.y / TAILLE_TUILE;
		auto min_tz = fenetre_donnees.min.z / TAILLE_TUILE;

		auto max_tx = converti_nombre_tuile(fenetre_donnees.max.x);
		auto max_ty = converti_nombre_tuile(fenetre_donnees.max.y);
		auto max_tz = converti_nombre_tuile(fenetre_donnees.max.z);

		for (auto z = min_tz; z < max_tz; ++z) {
			for (auto y = min_ty; y < max_ty; ++y) {
				for (auto x = min_tx; x < max_tx; ++x) {
					auto idx_tuile = index_tuile(x, y, z);

					if (m_index_tuiles[idx_tuile] != -1) {
						continue;
					}

					auto t = memoire::loge<type_tuile>("tuile");
					t->min = dls::math::vec3i(x, y, z) * TAILLE_TUILE;
					t->max = t->min + dls::math::vec3i(TAILLE_TUILE);

					m_index_tuiles[idx_tuile] = m_tuiles.taille();
					m_tuiles.pousse(t);
				}
			}
		}
	}

	void elague()
	{
		auto tuiles_gardees = dls::tableau<type_tuile *>();

		for (auto t : m_tuiles) {
			t->garde = false;

			for (auto i = 0; i < (VOXELS_TUILE); ++i) {
				if (t->donnees[i] != m_arriere_plan) {
					tuiles_gardees.pousse(t);
					t->garde = true;
					break;
				}
			}
		}

		auto suppr = 0;
		for (auto t : m_tuiles) {
			if (t->garde == false) {
				type_tuile::detruit(t);
				suppr++;
			}
		}

		m_tuiles = tuiles_gardees;

		for (auto i = 0; i < m_index_tuiles.taille(); ++i) {
			m_index_tuiles[i] = -1;
		}

		auto i = 0;
		for (auto t : m_tuiles) {
			auto idx_tuile = index_tuile(t->min.x / TAILLE_TUILE, t->min.y / TAILLE_TUILE, t->min.z / TAILLE_TUILE);
			m_index_tuiles[idx_tuile] = i++;
		}
	}

	type_valeur valeur(int i, int j, int k, float temps = 0.0f) const
	{
		/* ceci est redondant avec le code en dessous mais nécessaire pour
		 * éviter les coordonnées négatives entre -(TAILLE_TUILE - 1) et 0 qui
		 * deviennent 0 via la division par TAILLE_TUILE. */
		if (dls::math::hors_limites(dls::math::vec3i(i, j, k), dls::math::vec3i(0), this->m_desc.resolution - dls::math::vec3i(1))) {
			return m_arriere_plan;
		}

		/* trouve la tuile */
		auto it = i / TAILLE_TUILE;
		auto jt = j / TAILLE_TUILE;
		auto kt = k / TAILLE_TUILE;

		if (hors_des_limites(it, jt, kt)) {
			return m_arriere_plan;
		}

		auto idx_tuile = index_tuile(it, jt, kt);

		if (m_index_tuiles[idx_tuile] == -1) {
			return m_arriere_plan;
		}

		auto t = m_tuiles[m_index_tuiles[idx_tuile]];

		/* calcul l'index dans la tuile */
		auto xt = i - it * TAILLE_TUILE;
		auto yt = j - jt * TAILLE_TUILE;
		auto zt = k - kt * TAILLE_TUILE;

		return type_tuile::echantillonne(t, xt + (yt + zt * TAILLE_TUILE) * TAILLE_TUILE, temps);
	}

	type_tuile *tuile_par_index(long idx) const
	{
		auto idx_tuile = m_index_tuiles[idx];

		if (idx_tuile == -1) {
			return nullptr;
		}

		return m_tuiles[idx_tuile];
	}

	type_tuile *cree_tuile(dls::math::vec3i const &co)
	{
		auto idx = dls::math::calcul_index(co / TAILLE_TUILE, res_tuile());
		auto t = memoire::loge<type_tuile>("tuile");
		t->min = co;
		t->max = t->min + dls::math::vec3i(TAILLE_TUILE);
		m_index_tuiles[idx] = m_tuiles.taille();
		m_tuiles.pousse(t);
		return t;
	}

	long nombre_tuile() const
	{
		return m_tuiles.taille();
	}

	type_tuile *tuile(long idx)
	{
		return m_tuiles[idx];
	}

	type_tuile const *tuile(long idx) const
	{
		return m_tuiles[idx];
	}

	type_topologie const &topologie() const
	{
		return m_index_tuiles;
	}

	dls::math::vec3i res_tuile() const
	{
		return dls::math::vec3i(m_tuiles_x, m_tuiles_y, m_tuiles_z);
	}

	plage_tuile plage()
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile(d, f);
	}

	plage_tuile_const plage() const
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile_const(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile_const(d, f);
	}

	base_grille *copie() const override
	{
		auto grille = memoire::loge<grille_eparse<T, type_tuile>>("grille", desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_index_tuiles = this->m_index_tuiles;
		grille->m_tuiles.redimensionne(this->m_tuiles.taille());

		for (auto i = 0; i < this->m_tuiles.taille(); ++i) {
			auto atuile = this->m_tuiles[i];
			auto ntuile = memoire::loge<type_tuile>("tuile");
			ntuile->min = atuile->min;
			ntuile->max = atuile->max;

			type_tuile::copie_donnees(atuile, ntuile);

			grille->m_tuiles[i] = ntuile;
		}

		return grille;
	}

	bool est_eparse() const override
	{
		return true;
	}

	void permute(grille_eparse<T, type_tuile> &autre)
	{
		std::swap(m_desc.etendue, autre.m_desc.etendue);
		std::swap(m_desc.resolution, autre.m_desc.resolution);
		std::swap(m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(m_desc.taille_voxel, autre.m_desc.taille_voxel);

		m_index_tuiles.permute(autre.m_index_tuiles);
		m_tuiles.permute(autre.m_tuiles);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(m_tuiles_x, autre.m_tuiles_x);
		std::swap(m_tuiles_y, autre.m_tuiles_y);
		std::swap(m_tuiles_z, autre.m_tuiles_z);
	}
};

}  /* namespace wlk */
