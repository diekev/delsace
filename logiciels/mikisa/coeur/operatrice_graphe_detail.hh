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

#include "compileuse_lcc.hh"
#include "operatrice_corps.h"

namespace lcc {
struct donnees_fonction;
}

/* ajouter des nouvelles à la fin et ajourner les tableaux de paramètres
 * d'entrées et sorties */
enum {
	DETAIL_POINTS,
	DETAIL_VOXELS,
	DETAIL_PIXELS,
	DETAIL_TERRAIN,
	DETAIL_POSEIDON_GAZ,
	DETAIL_NUANCAGE,
};

extern lcc::param_sorties params_noeuds_entree[];
extern lcc::param_entrees params_noeuds_sortie[];

struct Mikisa;
struct Nuanceur;

/* ************************************************************************** */

class OperatriceGrapheDetail final : public OperatriceCorps {
	CompileuseGrapheLCC m_compileuse;

public:
	int type_detail = DETAIL_POINTS;

	static constexpr auto NOM = "Graphe Détail";
	static constexpr auto AIDE = "Graphe Détail";

	OperatriceGrapheDetail(Graphe &graphe_parent, Noeud &noeud_);

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	type_prise type_entree(int) const override;

	type_prise type_sortie(int) const override;

	int type() const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

private:
	res_exec execute_detail_corps(ContexteEvaluation const &contexte, DonneesAval *donnees_aval);

	res_exec execute_detail_pixel(ContexteEvaluation const &contexte, DonneesAval *donnees_aval);

	void execute_script_sur_points(ChefExecution *chef, const AccesseusePointLecture &points_entree, AccesseusePointEcriture *points_sortie);
};

/* ************************************************************************** */

class OperatriceFonctionDetail final : public OperatriceImage {
public:
	lcc::donnees_fonction const *donnees_fonction = nullptr;
	dls::chaine nom_fonction = "";

	static constexpr auto NOM = "Fonction Détail";
	static constexpr auto AIDE = "Fonction Détail";

	OperatriceFonctionDetail(Graphe &graphe_parent, Noeud &noeud_, dls::chaine const &nom_fonc, lcc::donnees_fonction const *df);

	OperatriceFonctionDetail(OperatriceFonctionDetail const &) = default;
	OperatriceFonctionDetail &operator=(OperatriceFonctionDetail const &) = default;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	const char *nom_entree(int i) override;

	const char *nom_sortie(int i) override;

	int type() const override;

	type_prise type_entree(int i) const override;

	type_prise type_sortie(int i) const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	/* ceci n'est appelé que lors des créations par l'utilisateur car les
	 * opératrices venant de sauvegardes ont déjà les propriétés créées */
	void cree_proprietes();

private:
	void cree_code_coulisse_processeur(
			compileuse_lng *compileuse,
			lcc::type_var type_specialise,
			dls::tableau<int> const &pointeurs);

	void cree_code_coulisse_opengl(
			DonneesAval *donnees_aval,
			lcc::type_var type_specialise,
			dls::tableau<int> const &pointeurs,
			int temps_courant);
};

/* ************************************************************************** */

bool compile_nuanceur_opengl(ContexteEvaluation const &contexte, Nuanceur &nuanceur);

OperatriceFonctionDetail *cree_op_detail(
		Mikisa &mikisa,
		Graphe &graphe,
		Noeud &noeud,
		dls::chaine const &nom_fonction);

void enregistre_operatrices_detail(UsineOperatrice &usine);
