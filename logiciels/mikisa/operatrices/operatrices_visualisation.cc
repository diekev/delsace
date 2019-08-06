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

#include "operatrices_visualisation.hh"

#include "corps/iteration_corps.hh"
#include "corps/limites_corps.hh"
#include "corps/volume.hh"

#include "coeur/delegue_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "arbre_octernaire.hh"
#include "outils_visualisation.hh"

/* ************************************************************************** */

class OperatriceVisualisationArbreOcternaire : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualisation Arbre Octernaire";
	static constexpr auto AIDE = "";

	OperatriceVisualisationArbreOcternaire(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points_pour_lecture();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}


		auto limites = calcule_limites_mondiales_corps(*corps_entree);

		auto arbre = ArbreOcternaire(limites);

		auto triangles = convertis_maillage_triangles(corps_entree, nullptr);

		for (auto const &triangle : triangles) {
			arbre.ajoute_triangle(triangle);
		}

		rassemble_topologie(arbre.racine(), m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto rassemble_topologie(ArbreHBE &arbre, Corps &corps)
{
	dls::math::vec3f couleurs[2] = {
		dls::math::vec3f(0.0f, 1.0f, 0.0f),
		dls::math::vec3f(0.0f, 0.0f, 1.0f),
	};

	auto attr_C = corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

	for (auto const &noeud : arbre.noeuds) {
		auto const &min = dls::math::vec3f(
					static_cast<float>(noeud.limites.min.x),
					static_cast<float>(noeud.limites.min.y),
					static_cast<float>(noeud.limites.min.z));

		auto const &max = dls::math::vec3f(
					static_cast<float>(noeud.limites.max.x),
					static_cast<float>(noeud.limites.max.y),
					static_cast<float>(noeud.limites.max.z));

		auto couleur = (noeud.est_feuille()) ? couleurs[0] : couleurs[1];

		dessine_boite(corps, attr_C, min, max, couleur);
	}
}

class OperatriceVisualisationArbreBVH : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualisation Arbre BVH";
	static constexpr auto AIDE = "";

	OperatriceVisualisationArbreBVH(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
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
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points_pour_lecture();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto delegue_prims = DeleguePrim(*corps_entree);
		auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

		rassemble_topologie(arbre_hbe, m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

template <typename T>
static auto visualise_topologie(Corps &corps, grille_eparse<T> const &grille)
{
	auto limites = grille.desc().etendue;
	auto attr_C = corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
	auto plg = grille.plage();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		auto min_tuile = grille.index_vers_monde(tuile->min);
		auto max_tuile = grille.index_vers_monde(tuile->max);

		dessine_boite(corps, attr_C, min_tuile, max_tuile, dls::math::vec3f(0.1f, 0.1f, 0.8f));
	}

	dessine_boite(corps, attr_C, limites.min, limites.max, dls::math::vec3f(0.1f, 0.8f, 0.1f));
}

class OpVisualiseGrilleEparse : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualisation Grille Éparse";
	static constexpr auto AIDE = "";

	OpVisualiseGrilleEparse(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
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
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		if (!possede_volume(*corps_entree)) {
			this->ajoute_avertissement("Le Corps ne possède pas de volume");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();
		auto volume = static_cast<Volume *>(nullptr);

		for (auto i = 0; i < prims->taille(); ++i) {
			auto prim = prims->prim(i);

			if (prim->type_prim() == type_primitive::VOLUME) {
				volume = dynamic_cast<Volume *>(prim);
			}
		}

		auto grille = volume->grille;

		if (!grille->est_eparse()) {
			this->ajoute_avertissement("Le volume n'est pas épars");
			return EXECUTION_ECHOUEE;
		}

		auto grille_eprs = dynamic_cast<grille_eparse<float> *>(grille);

		visualise_topologie(m_corps, *grille_eprs);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_visualisation(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceVisualisationArbreOcternaire>());
	usine.enregistre_type(cree_desc<OperatriceVisualisationArbreBVH>());
	usine.enregistre_type(cree_desc<OpVisualiseGrilleEparse>());
}
