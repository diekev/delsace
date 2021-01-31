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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/chaine.hh"

#include "coeur/operatrice_image.h"

struct Jorjala;

namespace ccl {
class NodeType;
class ShaderNode;
}

class OperatriceCycles final : public OperatriceImage {
public:
	const ccl::NodeType *type_noeud = nullptr;
	ccl::ShaderNode *noeud_cycles = nullptr;

	static constexpr auto NOM = "Fonction Détail";
	static constexpr auto AIDE = "Fonction Détail";

	OperatriceCycles(Graphe &graphe_parent, Noeud &noeud_, const ccl::NodeType *type_noeud_);

	OperatriceCycles(OperatriceCycles const &) = default;
	OperatriceCycles &operator=(OperatriceCycles const &) = default;

	static OperatriceCycles *cree(Graphe &graphe, Noeud &noeud, dls::chaine const &nom_type);

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	const char *nom_entree(int i) override;

	const char *nom_sortie(int i) override;

	int type() const override;

	type_prise type_entree(int i) const override;

	type_prise type_sortie(int i) const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

private:
	void cree_proprietes();
};

dls::chaine genere_menu_noeuds_cycles();
