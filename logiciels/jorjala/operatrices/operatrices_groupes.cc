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

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

struct ParamEchantGroupe {
	long depart = 0;
	long decalage = 0;
	long n = 0;
	float probabilite = 0.0f;
	int graine = 0;
	bool echantillonage_reservoir = false;
	REMBOURRE(7);
};

template <typename TypeGroupe>
void echantillonne_groupe(
		TypeGroupe &groupe,
		ParamEchantGroupe const &params)
{
	dls::tableau<long> index_possibles;
	index_possibles.reserve(params.n);

	for (auto i = params.depart; i < params.n; i += params.decalage) {
		index_possibles.ajoute(i);
	}

	/* à partir de là, 'n' doit être le nombre d'index possibles, qui peut être
	 * inférieur à params.n en cas de choix sur la moitié paire ou impaire */
	auto const n = index_possibles.taille();

	auto gna = GNA(static_cast<unsigned long>(params.graine));
	auto const k = static_cast<long>(static_cast<float>(n) * params.probabilite);
	groupe.reserve(k);

	if (params.echantillonage_reservoir) {
		/* utilise d'un algorithme d'échantillonage réservoir
		 * voir https://en.wikipedia.org/wiki/Reservoir_sampling */
		for (auto i = 0; i < k; ++i) {
			groupe.ajoute_index(index_possibles[i]);
		}

		for (auto i = k; i < n; ++i) {
			auto j = gna.uniforme(0l, i);

			if (j < k) {
				groupe.remplace_index(j, index_possibles[i]);
			}
		}
	}
	else {
		for (auto i : index_possibles) {
			if (gna.uniforme(0.0f, 1.0f) > params.probabilite) {
				continue;
			}

			groupe.ajoute_index(i);
		}
	}
}

class OperatriceCreationGroupe final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Groupe";
	static constexpr auto AIDE = "";

	OperatriceCreationGroupe(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const nom_groupe = evalue_chaine("nom_groupe");
		auto const contenu = evalue_enum("contenu");
		auto const chaine_methode = evalue_enum("méthode");

		if (nom_groupe.est_vide()) {
			ajoute_avertissement("Le nom du groupe est vide !");
			return res_exec::ECHOUEE;
		}

		auto params = ParamEchantGroupe{};
		params.graine = evalue_entier("graine", contexte.temps_courant);
		params.probabilite = evalue_decimal("probabilité", contexte.temps_courant);
		params.echantillonage_reservoir = evalue_bool("échantillonage_réservoir");

		if (chaine_methode == "tout") {
			params.depart = 0l;
			params.decalage = 1l;
		}
		else if (chaine_methode == "pair") {
			params.depart = 0l;
			params.decalage = 2l;
		}
		else if (chaine_methode == "impair") {
			params.depart = 1l;
			params.decalage = 2l;
		}
		else {
			ajoute_avertissement("La méthode '", chaine_methode, "' est invalide !");
			return res_exec::ECHOUEE;
		}

		/* création de tous les index possibles */
		if (contenu == "points") {
			params.n = m_corps.points_pour_lecture().taille();
		}
		else if (contenu == "primitives") {
			params.n = m_corps.prims()->taille();
		}
		else {
			ajoute_avertissement("Le contenu du groupe '", contenu, "' est invalide !");
			return res_exec::ECHOUEE;
		}

		if (contenu == "points") {
			auto groupe = m_corps.groupe_point(nom_groupe);

			if (groupe != nullptr) {
				ajoute_avertissement("Le groupe '", nom_groupe, "' existe déjà !");
				return res_exec::ECHOUEE;
			}

			groupe = m_corps.ajoute_groupe_point(nom_groupe);

			echantillonne_groupe(*groupe, params);
		}
		else if (contenu == "primitives") {
			auto groupe = m_corps.groupe_primitive(nom_groupe);

			if (groupe != nullptr) {
				ajoute_avertissement("Le groupe '", nom_groupe, "' existe déjà !");
				return res_exec::ECHOUEE;
			}

			groupe = m_corps.ajoute_groupe_primitive(nom_groupe);

			echantillonne_groupe(*groupe, params);
		}
		else {
			ajoute_avertissement("Le contenu du groupe '", contenu, "' est invalide !");
			return res_exec::ECHOUEE;
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_groupes(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationGroupe>());
}

#pragma clang diagnostic pop
