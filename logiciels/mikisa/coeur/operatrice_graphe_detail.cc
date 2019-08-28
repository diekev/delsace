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

#include "lcc/code_inst.hh"
#include "lcc/lcc.hh"

#include "contexte_evaluation.hh"
#include "donnees_aval.hh"
#include "objet.h"
#include "mikisa.h"
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

template <typename T>
static auto remplis_donnees(
		lcc::pile &donnees,
		gestionnaire_propriete &gest_props,
		dls::chaine const &nom,
		T const &v)
{
	auto idx = gest_props.pointeur_donnees(nom);

	if (idx == -1) {
		/* À FAIRE : erreur */
		return;
	}

	donnees.stocke(idx, v);
}

int OperatriceGrapheDetail::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (!this->entree(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return EXECUTION_ECHOUEE;
	}

	m_corps.reinitialise();
	entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

	compile_graphe(contexte);

	auto points = m_corps.points_pour_ecriture();

	/* fais une copie locale pour éviter les problèmes de concurrence critique */
	auto donnees = m_compileuse.donnees();

	auto ctx_exec = lcc::ctx_exec{};
	auto ctx_local = lcc::ctx_local{};

	for (auto i = 0; i < points->taille(); ++i) {
		auto pos = points->point(i);

		remplis_donnees(donnees, m_gest_props, "P", pos);

		lcc::execute_pile(
					ctx_exec,
					ctx_local,
					donnees,
					m_compileuse.instructions(),
					static_cast<int>(i));

		auto idx_sortie = m_gest_props.pointeur_donnees("P");
		pos = donnees.charge_vec3(idx_sortie);

		points->point(i, pos);
	}

	return EXECUTION_REUSSIE;
}

void OperatriceGrapheDetail::compile_graphe(const ContexteEvaluation &contexte)
{
	m_compileuse = compileuse_lng();
	m_gest_props = gestionnaire_propriete();

	if (m_graphe.besoin_ajournement) {
		tri_topologique(m_graphe);
		m_graphe.besoin_ajournement = false;
	}

	auto donnees_aval = DonneesAval{};
	donnees_aval.table.insere({"compileuse", &m_compileuse});
	donnees_aval.table.insere({"gest_props", &m_gest_props});

	auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(lcc::type_var::VEC3));
	m_gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);

	for (auto &noeud : m_graphe.noeuds()) {
		for (auto &sortie : noeud->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = extrait_opimage(noeud->donnees());
		operatrice->execute(contexte, &donnees_aval);
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
	/* réimplémentation du code de génération d'instruction pour les appels de
	 * fonctions de LCC */

	/* À FAIRE : surcharge pour les types polymorphiques */

	auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);

	/* cherche les pointeurs des entrées */

	auto pointeurs = dls::tableau<int>();

	for (auto i = 0; i < entrees(); ++i) {
		if (entree(i)->connectee()) {
			auto ptr = entree(i)->pointeur();
			auto sortie = ptr->liens[0];
			pointeurs.pousse(static_cast<int>(sortie->decalage_pile));
		}
		else {
			/* alloue une valeur par défaut et prend le pointeurs */
			/* À FAIRE : params interface */
			auto type = m_df->seing.entrees.types[i];
			auto ptr = compileuse->donnees().loge_donnees(lcc::taille_type(type));
			pointeurs.pousse(ptr);
		}
	}

	/* ****************** crée les données pour les appels ****************** */

	/* ajoute le code_inst de la fonction */
	compileuse->ajoute_instructions(m_df->type);

	/* ajoute le type de la fonction pour choisir la bonne surcharge */
//	if (type_instance != type_var::INVALIDE) {
//		compileuse->ajoute_instructions(type_instance);
//	}

	/* ajoute le pointeur de chaque paramètre */
	for (auto ptr : pointeurs) {
		compileuse->ajoute_instructions(ptr);
	}

	/* pour chaque sortie, nous réservons de la place sur la pile de données */
	auto pointeur_donnees = 0;
	for (auto i = 0; i < sorties(); ++i) {
		auto type = m_df->seing.sorties.types[i];
		auto pointeur = compileuse->donnees().loge_donnees(lcc::taille_type(type));
		auto ptr_sortie = sortie(i)->pointeur();

		ptr_sortie->decalage_pile = pointeur;

		if (i == 0) {
			pointeur_donnees = pointeur;
		}
	}

	/* ajoute le pointeur du premier paramètre aux instructions pour savoir
	 * où écrire */
	compileuse->ajoute_instructions(pointeur_donnees);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

class OperatriceEntreeDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Entrée Détail";
	static constexpr auto AIDE = "Entrée Détail";

	explicit OperatriceEntreeDetail(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(0);
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

	type_prise type_sortie(int i) const override
	{
		INUTILISE(i);
		return type_prise::VEC3;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto gest_props = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_props"]);
		sortie(0)->pointeur()->decalage_pile = gest_props->pointeur_donnees("P");

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSortieDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Sortie Détail";
	static constexpr auto AIDE = "Sortie Détail";

	explicit OperatriceSortieDetail(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
		sorties(0);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	type_prise type_entree(int i) const override
	{
		INUTILISE(i);
		return type_prise::VEC3;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto gest_props = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_props"]);

		auto ptr_entree = 0;

		if (entree(0)->connectee()) {
			auto ptr = entree(0)->pointeur();
			ptr_entree = static_cast<int>(ptr->liens[0]->decalage_pile);
		}
		else {
			ptr_entree = compileuse->donnees().loge_donnees(taille_type(lcc::type_var::VEC3));
		}

		auto ptr = gest_props->pointeur_donnees("P");

		compileuse->ajoute_instructions(lcc::code_inst::ASSIGNATION);
		compileuse->ajoute_instructions(lcc::type_var::VEC3);
		compileuse->ajoute_instructions(ptr_entree);
		compileuse->ajoute_instructions(ptr);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void graphe_detail_notifie_parent_suranne(Mikisa &mikisa)
{
	auto noeud_objet = mikisa.bdd.graphe_objets()->noeud_actif;
	auto objet = extrait_objet(noeud_objet->donnees());
	auto graphe = &objet->graphe;
	auto noeud_actif = graphe->noeud_actif;

	/* Marque le noeud courant et ceux en son aval surannées. */
	marque_surannee(noeud_actif, [](Noeud *n, PriseEntree *prise)
	{
		auto op = extrait_opimage(n->donnees());
		op->amont_change(prise);
	});
}

/* ************************************************************************** */

void enregistre_operatrices_detail(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGrapheDetail>());
	usine.enregistre_type(cree_desc<OperatriceEntreeDetail>());
	usine.enregistre_type(cree_desc<OperatriceSortieDetail>());
}
