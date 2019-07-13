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

#include "grille.h"

class Maillage;

struct Particule {
	dls::math::vec3f pos{};
	dls::math::vec3f vel{};
	dls::math::vec3f vel_pic{};
};

#if 0
static constexpr auto VOXELS_N0 = 8;
static constexpr auto VOXELS_N1 = 8;
static constexpr auto VOXELS_DECALAGE = VOXELS_N0 * VOXELS_N1;

class GrilleParticule {
	dls::tableau<size_t> m_niveau_0;
	dls::tableau<size_t> m_niveau_1;

	struct NoeudNiveau0 {
		NoeudNiveau1 *noeuds[VOXELS_N0];
	};

	struct NoeudNiveau1 {
		NoeudNiveau2 *noeuds[VOXELS_N1];
	};

	struct NoeudNiveau2 {
		dls::liste<Particule *> particules;
	};

public:
	const dls::liste<Particule *> &particules(size_t x, size_t y, size_t z)
	{
		auto const x0 = x / VOXELS_DECALAGE;
		auto const y0 = y / VOXELS_DECALAGE;
		auto const z0 = z / VOXELS_DECALAGE;

		auto const index_niveau0 = x0 + (y0 + z0 * VOXELS_N0) * VOXELS_N0;
	}
};
#else
class GrilleParticule {
	dls::tableau<dls::tableau<Particule *>> m_donnees = {};
	dls::math::vec3<long> m_res = dls::math::vec3<long>(0l, 0l, 0l);
	long m_nombre_voxels = 0;

	dls::tableau<Particule *> m_arriere_plan = {};

	long calcul_index(long x, long y, long z) const
	{
		return x + (y + z * m_res[1]) * m_res[0];
	}

	bool hors_des_limites(long x, long y, long z) const
	{
		if (x < 0 || x >= m_res[0]) {
			return true;
		}

		if (y < 0 || y >= m_res[1]) {
			return true;
		}

		if (z < 0 || z >= m_res[2]) {
			return true;
		}

		return false;
	}

public:
	void initialise(long res_x, long res_y, long res_z)
	{
		m_res.x = res_x;
		m_res.y = res_y;
		m_res.z = res_z;

		m_nombre_voxels = res_x * res_y * res_z;
		m_donnees.redimensionne(m_nombre_voxels);

		for (auto &donnees : m_donnees) {
			donnees.efface();
		}
	}

	void ajoute_particule(long x, long y, long z, Particule *particule)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[calcul_index(x, y, z)].pousse(particule);
	}

	const dls::tableau<Particule *> &particule(long x, long y, long z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[calcul_index(x, y, z)];
	}
};
#endif

enum {
	CELLULE_VIDE   = 0,
	CELLULE_FLUIDE = 1,
};

struct Fluide {
	Maillage *source{};
	Maillage *domaine{};

	dls::math::vec3<long> res{};

	Grille<char> drapeaux{};
	Grille<float> phi{};
	Grille<float> velocite_x{};
	Grille<float> velocite_y{};
	Grille<float> velocite_z{};
	Grille<float> velocite_x_ancienne{};
	Grille<float> velocite_y_ancienne{};
	Grille<float> velocite_z_ancienne{};
	Grille<dls::math::vec3f> velocite{};
	Grille<dls::math::vec3f> ancienne_velocites{};
	GrilleParticule grille_particules{};

	dls::tableau<Particule> particules{};

	Fluide();
	~Fluide();

	/* pour faire taire cppcheck car source et domaine sont alloués dynamiquement */
	Fluide(Fluide const &autre) = default;
	Fluide &operator=(Fluide const &autre) = default;

	int temps_courant{};
	int temps_debut{};
	int temps_fin{};
	int temps_precedent{};

	void ajourne_pour_nouveau_temps();

private:
	void initialise();
	void construit_grille_particule();
	void soustrait_velocite();
	void sauvegarde_velocite_PIC();
	void transfert_velocite();
	void advecte_particules();
	void ajoute_acceleration();
	void conditions_bordure();
	void construit_champs_distance();
	void etend_champs_velocite();
};

void cree_particule(Fluide *fluide, long nombre);
