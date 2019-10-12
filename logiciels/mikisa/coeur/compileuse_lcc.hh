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

#include "lcc/contexte_generation_code.h"

struct ContexteEvaluation;
struct Corps;
struct Graphe;

/* ************************************************************************** */

struct CompileuseLCC {
	compileuse_lng m_compileuse{};
	gestionnaire_propriete m_gest_attrs{};
	lcc::ctx_exec m_ctx_global{};

	lcc::pile &donnees();

	template <typename T>
	void remplis_donnees(
			lcc::pile &donnees_pile,
			dls::chaine const &nom,
			T const &valeur)
	{
		::remplis_donnees(donnees_pile, m_gest_attrs, nom, valeur);
	}

	void stocke_attributs(lcc::pile &donnees, long idx_attr);

	void charge_attributs(lcc::pile &donnees, long idx_attr);

	int pointeur_donnees(dls::chaine const &nom);

	void execute_pile(lcc::ctx_local &ctx_local, lcc::pile &donnees_pile);
};

/* ************************************************************************** */

struct CompileuseGrapheLCC : public CompileuseLCC {
	Graphe &graphe;

	CompileuseGrapheLCC(Graphe &ptr_graphe);

	bool compile_graphe(ContexteEvaluation const &contexte, Corps *corps);
};
