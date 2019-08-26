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

#include "operatrice_graphe_detail.hh"

#include "lcc/lcc.hh"

#include "contexte_evaluation.hh"
#include "usine_operatrice.h"

/* ************************************************************************** */

OperatriceGrapheDetail::OperatriceGrapheDetail(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);
}

const char *OperatriceGrapheDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceGrapheDetail::texte_aide() const
{
	return AIDE;
}

const char *OperatriceGrapheDetail::chemin_entreface() const
{
	return "";
}

Graphe *OperatriceGrapheDetail::graphe()
{
	return &m_graphe;
}

int OperatriceGrapheDetail::type() const
{
	return OPERATRICE_GRAPHE_DETAIL;
}

int OperatriceGrapheDetail::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (!this->entree(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return EXECUTION_ECHOUEE;
	}

	m_corps.reinitialise();
	entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

	compile_graphe(contexte.temps_courant);

	/* fais une copie locale pour éviter les problèmes de concurrence critique */
	auto pile = m_compileuse.pile();
	auto points = m_corps.points_pour_ecriture();

	for (auto i = 0; i < points->taille(); ++i) {
		auto pos = points->point(i);

	//	execute_graphe(pile.debut(), pile.fin(), m_gestionnaire, pos, pos);

		points->point(i, pos);
	}

	return EXECUTION_REUSSIE;
}

void OperatriceGrapheDetail::compile_graphe(int temps)
{
	m_compileuse = CompileuseGraphe();
	//m_gestionnaire.reinitialise();

	if (m_graphe.besoin_ajournement) {
		tri_topologique(m_graphe);
		m_graphe.besoin_ajournement = false;
	}

	for (auto &noeud : m_graphe.noeuds()) {
		for (auto &sortie : noeud->sorties()) {
			sortie->decalage_pile = 0;
		}

//		auto operatrice = extrait_opimage(noeud->donnees());
//		auto operatrice_p3d = dynamic_cast<OperatricePoint3D *>(operatrice);

//		if (operatrice_p3d == nullptr) {
//			ajoute_avertissement("Impossible de trouver une opératrice point 3D dans le graphe !");
//			return;
//		}

//		operatrice_p3d->compile(m_compileuse, m_gestionnaire, temps);
	}
}

/* ************************************************************************** */

static auto converti_type_prise(lcc::type_var type)
{
	switch (type) {
		case lcc::type_var::DEC:
			return type_prise::DECIMAL;
		case lcc::type_var::ENT32:
			return type_prise::ENTIER;
		case lcc::type_var::VEC2:
			return type_prise::VEC2;
		case lcc::type_var::VEC3:
			return type_prise::VEC3;
		case lcc::type_var::VEC4:
			return type_prise::VEC4;
		case lcc::type_var::COULEUR:
			return type_prise::COULEUR;
		case lcc::type_var::MAT3:
			return type_prise::MAT3;
		case lcc::type_var::MAT4:
			return type_prise::MAT4;
		case lcc::type_var::CHAINE:
			return type_prise::CHAINE;
		case lcc::type_var::INVALIDE:
			return type_prise::INVALIDE;
		case lcc::type_var::TABLEAU:
			return type_prise::TABLEAU;
		case lcc::type_var::POLYMORPHIQUE:
			return type_prise::POLYMORPHIQUE;
	}

	return type_prise::INVALIDE;
}

OperatriceFonctionDetail::OperatriceFonctionDetail(Graphe &graphe_parent, Noeud *noeud, const lcc::donnees_fonction *df)
	: OperatriceImage(graphe_parent, noeud)
	, m_df(df)
{
	if (m_df == nullptr) {
		entrees(0);
		sorties(0);
		return;
	}

	entrees(m_df->seing.entrees.types.taille());
	sorties(m_df->seing.sorties.types.taille());
}

const char *OperatriceFonctionDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceFonctionDetail::texte_aide() const
{
	return AIDE;
}

const char *OperatriceFonctionDetail::chemin_entreface() const
{
	return "";
}

type_prise OperatriceFonctionDetail::type_entree(int i) const
{
	return converti_type_prise(m_df->seing.entrees.types[i]);
}

type_prise OperatriceFonctionDetail::type_sortie(int i) const
{
	return converti_type_prise(m_df->seing.sorties.types[i]);
}

int OperatriceFonctionDetail::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	INUTILISE(contexte);
	INUTILISE(donnees_aval);
	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

void enregistre_operatrices_detail(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGrapheDetail>());
}
