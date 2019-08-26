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

#include "operatrice_corps.h"

#include "biblinternes/graphe/compileuse_graphe.h"
#include "biblinternes/graphe/graphe.h"

namespace lcc {
struct donnees_fonction;
}

/* ************************************************************************** */

class OperatriceGrapheDetail : public OperatriceCorps {
	//GestionnaireDonneesGraphe m_gestionnaire{}; // ancien gestionnaire pour stocker des bruits et autres
	CompileuseGraphe m_compileuse{};
	Graphe m_graphe;

public:
	static constexpr auto NOM = "Graphe Détail";
	static constexpr auto AIDE = "Graphe Détail";

	explicit OperatriceGrapheDetail(Graphe &graphe_parent, Noeud *noeud);

	virtual const char *nom_classe() const override;

	virtual const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	int type_entree(int) const override;

	int type_sortie(int) const override;

	Graphe *graphe();

	virtual int type() const override;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	void compile_graphe(int temps);
};

/* ************************************************************************** */

class OperatriceFonctionDetail : public OperatriceImage {
	lcc::donnees_fonction const *m_df = nullptr;

public:
	static constexpr auto NOM = "Fonction Détail";
	static constexpr auto AIDE = "Fonction Détail";

	explicit OperatriceFonctionDetail(Graphe &graphe_parent, Noeud *noeud, lcc::donnees_fonction const *df);

	virtual const char *nom_classe() const override;

	virtual const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	int type_entree(int i) const override;

	int type_sortie(int i) const override;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;
};

/* ************************************************************************** */

void enregistre_operatrices_detail(UsineOperatrice &usine);
