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

#include "operatrices_maillage.hh"

#include <set>

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "courbure.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCourbureMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Courbure Maillage";
	static constexpr auto AIDE = "Calcul la courbure du maillage d'entrée";

	OperatriceCourbureMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_courbure_maillage.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto points_entree = m_corps.points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto relatif = evalue_bool("relatif");
		auto courbure_max = evalue_decimal("courbure_max");
		auto rayon = evalue_decimal("rayon");

		auto donnees_ret = calcule_courbure(
					m_corps,
					relatif,
					static_cast<double>(rayon),
					static_cast<double>(courbure_max));

		if (donnees_ret.nombre_instable > 0) {
			std::stringstream ss;
			ss << "Il y a " << donnees_ret.nombre_instable
			   << " points instables. Veuillez modifier le rayon pour stabiliser l'algorithme.";
			this->ajoute_avertissement(ss.str());
		}

		if (donnees_ret.nombre_impossible > 0) {
			std::stringstream ss;
			ss << "Il y a " << donnees_ret.nombre_impossible
			   << " points impossibles à calculer. Veuillez modifier le rayon pour stabiliser l'algorithme.";
			this->ajoute_avertissement(ss.str());
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceLissageLaplacien : public OperatriceCorps {
public:
	static constexpr auto NOM = "Lissage Laplacien";
	static constexpr auto AIDE = "Performe un lissage laplacien des points du corps d'entrée.";

	OperatriceLissageLaplacien(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_lissage_laplacien.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, int temps) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(rectangle, temps);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		corps_entree->copie_vers(&m_corps);

		/* Calcul le voisinage. */
		std::vector<std::set<long>> voisins(static_cast<size_t>(points_entree->taille()));

		for (auto i = 0; i < prims_entree->taille(); ++i) {
			auto prim = prims_entree->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::FERME) {
				continue;
			}

			for (auto j = 0; j < polygone->nombre_sommets() - 1; ++j) {
				auto i0 = polygone->index_point(j);
				auto i1 = polygone->index_point(j + 1);

				voisins[static_cast<size_t>(i0)].insert(i1);
				voisins[static_cast<size_t>(i1)].insert(i0);
			}

			auto dernier = polygone->index_point(polygone->nombre_sommets() - 1);
			auto premier = polygone->index_point(0);

			voisins[static_cast<size_t>(premier)].insert(dernier);
			voisins[static_cast<size_t>(dernier)].insert(premier);
		}

		/* Calcul les nouvelles positions. */
		auto pondere_distance = evalue_bool("pondère_distance");
		auto iterations = evalue_entier("itérations");

		std::vector<dls::math::vec3f> points_tmp(static_cast<size_t>(points_entree->taille()));

		for (auto k = 0; k < iterations; ++k) {
			for (auto i = 0; i < points_entree->taille(); ++i) {
				points_tmp[static_cast<size_t>(i)] = m_corps.points()->point(i);
			}

			if (pondere_distance) {
				for (auto i = 0ul; i < points_tmp.size(); ++i) {
					auto p = dls::math::vec3f(0.0f);
					auto const &xi = points_tmp[i];
					auto poids = 1.0f;

					for (auto j : voisins[i]) {
						auto xj = points_tmp[static_cast<size_t>(j)];
						auto ai = 1.0f / longueur(xi - xj);

						p += ai * xj;
						poids += ai;
					}

					p /= poids;
					m_corps.points()->point(static_cast<long>(i), p);
				}
			}
			else {
				for (auto i = 0ul; i < points_tmp.size(); ++i) {
					auto p = dls::math::vec3f(0.0f);

					for (auto j : voisins[i]) {
						p += points_tmp[static_cast<size_t>(j)];
					}

					p /= static_cast<float>(voisins[i].size());
					m_corps.points()->point(static_cast<long>(i), p);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_maillage(UsineOperatrice &usine)
{

	usine.enregistre_type(cree_desc<OperatriceCourbureMaillage>());
	usine.enregistre_type(cree_desc<OperatriceLissageLaplacien>());
}

#pragma clang diagnostic pop
