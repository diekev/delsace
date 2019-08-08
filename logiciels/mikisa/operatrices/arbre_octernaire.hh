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
#include "biblinternes/structures/tableau.hh"

/**
 * Structure auxilliaire pour générer les limites des noeuds enfants selon les
 * limites du noeud parent. Nécessaire pour pouvoir générer un arbre N-aire
 * selon le type de vecteur (à savoir les dimensions).
 */
template <typename type_vecteur>
struct decoupeuse_limites;

template <>
struct decoupeuse_limites<dls::math::vec3f> {
	limites3f limites[8];

	explicit decoupeuse_limites(limites3f lmt)
	{
		auto const &min = lmt.min;
		auto const &max = lmt.max;

		auto centre = min + (max - min) * 0.5f;

		limites[0] = limites3f(min, centre);
		limites[1] = limites3f(dls::math::vec3f(centre.x, min.y, min.z), dls::math::vec3f(max.x, centre.y, centre.z));
		limites[2] = limites3f(dls::math::vec3f(centre.x, min.y, centre.z), dls::math::vec3f(max.x, centre.y, max.z));
		limites[3] = limites3f(dls::math::vec3f(min.x, min.y, centre.z), dls::math::vec3f(centre.x, centre.y, max.z));
		limites[4] = limites3f(dls::math::vec3f(min.x, centre.y, min.z), dls::math::vec3f(centre.x, max.y, centre.z));
		limites[5] = limites3f(dls::math::vec3f(centre.x, centre.y, min.z), dls::math::vec3f(max.x, max.y, centre.z));
		limites[6] = limites3f(centre, max);
		limites[7] = limites3f(dls::math::vec3f(min.x, centre.y, centre.z), dls::math::vec3f(centre.x, max.y, max.z));
	}
};

/**
 * Arbre N-aire permettant de partitionner l'espace en sous-parties égales et
 * d'y stocker des éléments selon leurs limites (boites englobantes).
 *
 * Le paramètre type_vecteur permet de définir le nombre de dimensions de
 * l'arbre.
 */
template <typename type_vecteur>
struct arbre_Naire {
	static constexpr auto NOMBRE_ENFANTS = 1 << type_vecteur::nombre_composants;
	using type_limites = limites<type_vecteur>;

	struct noeud {
		type_limites limites{};

		int profondeur = 0;
		bool est_feuille = false;
		bool pad[3];

		type_limites limites_enfants[NOMBRE_ENFANTS];
		noeud *enfants[NOMBRE_ENFANTS];

		dls::tableau<long> refs{};

		noeud()
		{
			for (auto i = 0; i < NOMBRE_ENFANTS; ++i) {
				enfants[i] = nullptr;
			}
		}

		noeud(noeud const &) = default;
		noeud &operator=(noeud const &) = default;

		~noeud()
		{
			for (auto i = 0; i < NOMBRE_ENFANTS; ++i) {
				memoire::deloge("arbre_Naire::noeud", enfants[i]);
			}
		}
	};

private:
	int m_profondeur_max = 0;
	noeud m_racine{};

public:
	/**
	 * Construction d'un arbre via un délégué qui s'occupe de définir le nombre
	 * d'éléments à insérer ainsi que leurs limites.
	 *
	 * Le délégué doit avoir l'interface suivante :
	 * struct délégué {
	 *	   long nombre_elements() const;
	 *     limites<type_vecteur> calcule_limites() const;
	 *     limites<type_vecteur> limites_globales() const;
	 * };
	 */
	template <typename TypeDelegue>
	static arbre_Naire construit(TypeDelegue const &delegue, int profondeur_max = 4)
	{
		auto arbre = arbre_Naire(delegue.limites_globales(), profondeur_max);

		for (auto i = 0; i < delegue.nombre_elements(); ++i) {
			auto limites = delegue.calcule_limites(i);

			arbre.insert_element(&arbre.m_racine, i, limites);
		}

		return arbre;
	}

	noeud const *racine() const
	{
		return &m_racine;
	}

private:
	arbre_Naire(type_limites const &limites, int profondeur_max)
		: m_profondeur_max(profondeur_max)
	{
		m_racine.limites = limites;
		m_racine.profondeur = 0;
		m_racine.est_feuille = false;

		construit_limites_enfants(&m_racine);
	}

	void construit_limites_enfants(noeud *n)
	{
		auto decoupeuse = decoupeuse_limites<type_vecteur>(n->limites);

		for (auto i = 0; i < 8; ++i) {
			n->limites_enfants[i] = decoupeuse.limites[i];
		}
	}

	void insert_element(noeud *racine, long idx, type_limites const &limites)
	{
		if (racine->est_feuille) {
			racine->refs.pousse(idx);
			return;
		}

		/* Ajoute de l'éléments dans toutes les feuilles le contenant. */
		for (auto i = 0; i < NOMBRE_ENFANTS; ++i) {
			auto limite_enfant = racine->limites_enfants[i];

			if (!limite_enfant.chevauchent(limites)) {
				continue;
			}

			auto enfant = racine->enfants[i];

			if (enfant == nullptr) {
				enfant = memoire::loge<arbre_Naire::noeud>("arbre_Naire::noeud");
				enfant->limites = limite_enfant;
				enfant->profondeur = racine->profondeur + 1;
				enfant->est_feuille = (enfant->profondeur == m_profondeur_max);
				construit_limites_enfants(enfant);

				racine->enfants[i] = enfant;
			}

			insert_element(enfant, idx, limites);
		}
	}
};

using arbre_octernaire = arbre_Naire<dls::math::vec3f>;
