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

#include "operatrices_groupes.hh"

#include "bibliotheques/outils/gna.hh"

#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCreationGroupe : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Groupe";
	static constexpr auto AIDE = "";

	OperatriceCreationGroupe(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_groupe.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const nom_groupe = evalue_chaine("nom_groupe");
		auto const contenu = evalue_enum("contenu");
		auto const chaine_methode = evalue_enum("méthode");
		auto const probabilite = evalue_decimal("probabilité", contexte.temps_courant);
		auto const graine = evalue_entier("graine", contexte.temps_courant);

		if (nom_groupe.empty()) {
			ajoute_avertissement("Le nom du groupe est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto depart = 0l;
		auto decalage = 1l;
		auto mult = probabilite;

		if (chaine_methode == "tout") {
			depart = 0l;
			decalage = 1l;
		}
		else if (chaine_methode == "pair") {
			depart = 0l;
			decalage = 2l;
		}
		else if (chaine_methode == "impair") {
			depart = 1l;
			decalage = 2l;
		}
		else {
			std::stringstream ss;
			ss << "La méthode '" << chaine_methode << "' est invalide !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		auto gna = GNA(graine);

		auto const echantillonage_reservoir = false; // À FAIRE : CRASH
		auto n = 0l;

		/* création de tous les index possibles */
		if (contenu == "points") {
			n = m_corps.points()->taille();
		}
		else if (contenu == "primitives") {
			n = m_corps.prims()->taille();
		}
		else {
			std::stringstream ss;
			ss << "Le contenu du groupe '" << contenu << "' est invalide !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		std::vector<size_t> index_possibles;
		index_possibles.reserve(static_cast<size_t>(n));

		for (auto i = depart; i < n; i += decalage) {
			index_possibles.push_back(static_cast<size_t>(i));
		}

		if (contenu == "points") {
			auto groupe = m_corps.groupe_point(nom_groupe);

			if (groupe != nullptr) {
				std::stringstream ss;
				ss << "Le groupe '" << nom_groupe << "' existe déjà !";
				ajoute_avertissement(ss.str());
				return EXECUTION_ECHOUEE;
			}

			groupe = m_corps.ajoute_groupe_point(nom_groupe);

			auto const k = static_cast<long>(static_cast<float>(n) * mult);
			groupe->reserve(k);

			if (echantillonage_reservoir) {
				// Rempli le réservoir
				for (auto i = 0; i < k; ++i) {
					groupe->ajoute_point(index_possibles[static_cast<size_t>(i)]);
				}

				// Remplace les éléments avec une probabilité descendante
				for (auto i = k; i < n; ++i) {
					auto j = gna.uniforme(0l, i);

					if (j < k) {
						groupe->remplace_index(static_cast<size_t>(j), index_possibles[static_cast<size_t>(i)]);
					}
				}
			}
			else {
				for (auto i = depart; i < n; i += decalage) {
					if (gna.uniforme(0.0f, 1.0f) > probabilite) {
						continue;
					}

					groupe->ajoute_point(static_cast<size_t>(i));
				}
			}
		}
		else if (contenu == "primitives") {
			auto groupe = m_corps.groupe_primitive(nom_groupe);

			if (groupe != nullptr) {
				std::stringstream ss;
				ss << "Le groupe '" << nom_groupe << "' existe déjà !";
				ajoute_avertissement(ss.str());
				return EXECUTION_ECHOUEE;
			}

			groupe = m_corps.ajoute_groupe_primitive(nom_groupe);

			auto const k = static_cast<long>(static_cast<float>(n) * mult);
			groupe->reserve(k);

			if (echantillonage_reservoir) {
				// Rempli le réservoir
				for (auto i = 0; i < k; ++i) {
					groupe->ajoute_primitive(index_possibles[static_cast<size_t>(i)]);
				}

				// Remplace les éléments avec une probabilité descendante
				for (auto i = k; i < n; ++i) {
					auto j = gna.uniforme(0l, i);

					if (j < k) {
						groupe->remplace_index(static_cast<size_t>(j), index_possibles[static_cast<size_t>(i)]);
					}
				}
			}
			else {
				for (auto i = depart; i < n; i += decalage) {
					if (gna.uniforme(0.0f, 1.0f) > probabilite) {
						continue;
					}

					groupe->ajoute_primitive(static_cast<size_t>(i));
				}
			}
		}
		else {
			std::stringstream ss;
			ss << "Le contenu du groupe '" << contenu << "' est invalide !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_groupes(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationGroupe>());
}

#pragma clang diagnostic pop
