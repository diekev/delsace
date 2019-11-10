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
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/ramasse_miettes.hh"
#include "biblinternes/structures/tableau.hh"

#include "wolika/grille_dense.hh"

namespace psn {

struct particules {
	using type_index = long;
	using type_scalaire = float;
	using type_vecteur = dls::math::vec3f;

	enum class type_champs {
		R32,
		VEC3,
	};

	struct champs {
		dls::tableau<char> donnees{};
		dls::chaine nom{};
		type_champs type{};
		int pad{};

		void redimensionne(type_index ntaille)
		{
			auto n = 0l;

			switch (type) {
				case type_champs::R32:
				{
					n = ntaille * static_cast<long>(sizeof(type_scalaire));
					break;
				}
				case type_champs::VEC3:
				{
					n = ntaille * static_cast<long>(sizeof(type_vecteur));
					break;
				}
			}

			donnees.redimensionne(n);
		}
	};

private:
	dls::tableau<champs> m_champs{};
	dls::ramasse_miette<type_index> m_ramasse_miettes{-1};
	type_index m_nombre_particules{};

	champs *trouve_champs(dls::chaine const &nom)
	{
		for (auto &chm : m_champs) {
			if (chm.nom == nom) {
				return &chm;
			}
		}

		return nullptr;
	}

	champs const *trouve_champs(dls::chaine const &nom) const
	{
		for (auto &chm : m_champs) {
			if (chm.nom == nom) {
				return &chm;
			}
		}

		return nullptr;
	}

public:
	static particules construit_systeme_gaz()
	{
		auto parts = particules();

		auto requiers_temperature = false;
		auto requiers_couleur = false;
		auto requiers_divergence = false;
		auto requiers_feu = false;

		parts.ajoute_champs("position", type_champs::VEC3);
		//parts.ajoute_champs("position_prev", type_champs::VEC3);
		//parts.ajoute_champs("vélocité", type_champs::VEC3);
		//parts.ajoute_champs("vélocité_prev", type_champs::VEC3);
		parts.ajoute_champs("densité", type_champs::R32);
		//parts.ajoute_champs("pression", type_champs::R32);

		if (requiers_temperature) {
			parts.ajoute_champs("température", type_champs::R32);
		}

		if (requiers_divergence) {
			parts.ajoute_champs("divergence", type_champs::R32);
		}

		if (requiers_feu) {
			parts.ajoute_champs("fioul", type_champs::R32);
			parts.ajoute_champs("réaction", type_champs::R32);
			parts.ajoute_champs("feu", type_champs::R32);
		}

		if (requiers_couleur) {
			parts.ajoute_champs("couleur", type_champs::VEC3);
		}

		return parts;
	}

	void efface()
	{
		for (auto &chm : m_champs) {
			chm.donnees.efface();
		}

		m_nombre_particules = 0;
		m_ramasse_miettes.efface();
	}

	void ajoute_champs(dls::chaine const &nom, type_champs type)
	{
		auto chm = champs{};
		chm.nom = nom;
		chm.type = type;

		m_champs.pousse(chm);
	}

	type_index ajoute_particule()
	{
		auto index = m_ramasse_miettes.trouve_miette();

		if (index == -1) {
			index = m_nombre_particules;
			m_nombre_particules += 1;

			for (auto &chm : m_champs) {
				chm.redimensionne(m_nombre_particules);
			}
		}

		return index;
	}

	void enleve_particule(type_index idx)
	{
		m_ramasse_miettes.ajoute_miette(idx);
	}

	void compresse()
	{
		/* À FAIRE */
	}

	type_index taille() const
	{
		return m_nombre_particules;
	}

	type_scalaire *champs_scalaire(dls::chaine const &nom)
	{
		auto chm = trouve_champs(nom);

		if (chm == nullptr) {
			return nullptr;
		}

		return reinterpret_cast<type_scalaire *>(chm->donnees.donnees());
	}

	type_scalaire const *champs_scalaire(dls::chaine const &nom) const
	{
		auto chm = trouve_champs(nom);

		if (chm == nullptr) {
			return nullptr;
		}

		return reinterpret_cast<type_scalaire const *>(chm->donnees.donnees());
	}

	type_vecteur *champs_vectoriel(dls::chaine const &nom)
	{
		auto chm = trouve_champs(nom);

		if (chm == nullptr) {
			return nullptr;
		}

		return reinterpret_cast<type_vecteur *>(chm->donnees.donnees());
	}

	type_vecteur const *champs_vectoriel(dls::chaine const &nom) const
	{
		auto chm = trouve_champs(nom);

		if (chm == nullptr) {
			return nullptr;
		}

		return reinterpret_cast<type_vecteur const *>(chm->donnees.donnees());
	}
};

struct GrilleParticule {
private:
	wlk::grille_dense_3d<int> m_grille{};
	dls::tableau< dls::tableau<int>> m_cellules{};

public:
	GrilleParticule() = default;

	GrilleParticule(wlk::desc_grille_3d const &desc)
		: m_grille(desc, -1)
	{}

	dls::tableau<int> &cellule(long idx)
	{
		auto idx_cellule = m_grille.valeur(idx);

		if (idx_cellule >= 0) {
			return m_cellules[idx_cellule];
		}

		m_grille.valeur(idx) = static_cast<int>(m_cellules.taille());
		m_cellules.pousse({});

		return m_cellules.back();
	}

	dls::tableau<int> voisines_cellules(
			const dls::math::vec3i& index,
			const dls::math::vec3i& numberOfNeighbors)
	{
		//loop through neighbors, for each neighbor, check if cell has particles and push back contents
		dls::tableau<int> neighbors;

		auto ix = index.x;
		auto iy = index.y;
		auto iz = index.z;

		auto nvx = numberOfNeighbors.x;
		auto nvy = numberOfNeighbors.y;
		auto nvz = numberOfNeighbors.z;

		auto res = m_grille.desc().resolution;

		auto limites = limites3i(dls::math::vec3i(0), res - dls::math::vec3i(1));
		auto pos = dls::math::vec3i();

		for (pos.x = ix - numberOfNeighbors.x; pos.x <= ix + nvx; pos.x += 1) {
			for (pos.y = iy - numberOfNeighbors.y; pos.y <= iy + nvy; pos.y += 1) {
				for (pos.z = iz - numberOfNeighbors.z; pos.z <= iz + nvz; pos.z += 1) {
					if (hors_limites(pos, limites)) {
						continue;
					}

					auto cellindex = m_grille.valeur(pos);

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

	void tri(particules const &parts)
	{
		for (auto &cellule : m_cellules) {
			cellule.efface();
		}

		auto pos = parts.champs_vectoriel("position");

		for (auto i = 0; i < parts.taille(); ++i) {
			auto pos_idx = m_grille.monde_vers_index(pos[i]);
			auto idx = m_grille.calcul_index(pos_idx);
			auto &cellule_idx = this->cellule(idx);
			cellule_idx.pousse(i);
		}
	}
};

struct Poseidon;

void transfere_particules_grille(Poseidon &poseidon);

}  /* namespace psn */
