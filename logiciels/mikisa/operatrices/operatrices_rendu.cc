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

#include "operatrices_rendu.hh"

#include "biblinternes/vision/camera.h"

#include "coeur/base_de_donnees.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#include "rendu/moteur_rendu.hh"
#include "rendu/moteur_rendu_koudou.hh"
#include "rendu/moteur_rendu_opengl.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static inline auto extrait_moteur_rendu(DonneesAval *da)
{
	return std::any_cast<MoteurRendu *>(da->table["moteur_rendu"]);
}

/* ************************************************************************** */

class OpRenduChercheObjets : public OperatriceImage {
public:
	static constexpr auto NOM = "Cherche Objets";
	static constexpr auto AIDE = "";

	OpRenduChercheObjets(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
		m_execute_toujours = true;
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
		if (!donnees_aval || !donnees_aval->possede("moteur_rendu")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		auto moteur_rendu = extrait_moteur_rendu(donnees_aval);
		auto delegue = moteur_rendu->delegue();

		delegue->objets.efface();

		for (auto objet : contexte.bdd->objets()) {
			delegue->objets.pousse(objet);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpMoteurRendu : public OperatriceImage {
	MoteurRendu *m_moteur_rendu = nullptr;

public:
	static constexpr auto NOM = "Moteur Rendu";
	static constexpr auto AIDE = "";

	OpMoteurRendu(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);		
		noeud.est_sortie = true;
	}

	COPIE_CONSTRUCT(OpMoteurRendu);

	~OpMoteurRendu() override
	{
		if (m_moteur_rendu == nullptr) {
			return;
		}

		if (m_moteur_rendu->id() == dls::chaine("opengl")) {
			auto moteur_rendu = dynamic_cast<MoteurRenduOpenGL *>(m_moteur_rendu);
			memoire::deloge("MoteurRenduOpenGL", moteur_rendu);
		}
		else if (m_moteur_rendu->id() == dls::chaine("koudou")) {
			auto moteur_rendu = dynamic_cast<MoteurRenduKoudou *>(m_moteur_rendu);
			memoire::deloge("MoteurRenduKoudou", moteur_rendu);
		}
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
		INUTILISE(donnees_aval);

		if (m_moteur_rendu == nullptr) {
			m_moteur_rendu = memoire::loge<MoteurRenduOpenGL>("MoteurRenduOpenGL");
		}

		auto da = DonneesAval{};
		da.table.insere({ "moteur_rendu", m_moteur_rendu });

		entree(0)->requiers_image(contexte, &da);

		auto tampon = contexte.tampon_rendu;
		auto camera = contexte.camera_rendu;
		auto &stats = contexte.stats_rendu;

		m_moteur_rendu->camera(camera);

		m_moteur_rendu->calcule_rendu(
					*stats,
					tampon,
					camera->hauteur(),
					camera->largeur(),
					contexte.rendu_final);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_rendu(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpRenduChercheObjets>());
	usine.enregistre_type(cree_desc<OpMoteurRendu>());
}

#pragma clang diagnostic pop
