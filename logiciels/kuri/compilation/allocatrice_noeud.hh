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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "arbre_syntaxique.hh"

struct AllocatriceNoeud {
	tableau_page<NoeudExpression> m_noeuds_expression{};
	tableau_page<NoeudDeclarationVariable> m_noeuds_declaration_variable{};
	tableau_page<NoeudExpressionReference> m_noeuds_expression_reference{};
	tableau_page<NoeudExpressionUnaire> m_noeuds_expression_unaire{};
	tableau_page<NoeudExpressionBinaire> m_noeuds_expression_binaire{};
	tableau_page<NoeudExpressionLogement> m_noeuds_expression_logement{};
	tableau_page<NoeudExpressionMembre> m_noeuds_expression_membre{};
	tableau_page<NoeudDeclarationCorpsFonction> m_noeuds_declaration_corps_fonction{};
	tableau_page<NoeudDeclarationEnteteFonction> m_noeuds_declaration_entete_fonction{};
	tableau_page<NoeudStruct> m_noeuds_struct{};
	tableau_page<NoeudEnum> m_noeuds_enum{};
	tableau_page<NoeudSi> m_noeuds_si{};
	tableau_page<NoeudSiStatique> m_noeuds_si_statique{};
	tableau_page<NoeudPour> m_noeuds_pour{};
	tableau_page<NoeudBoucle> m_noeuds_boucle{};
	tableau_page<NoeudBloc> m_noeuds_bloc{};
	tableau_page<NoeudDiscr> m_noeuds_discr{};
	tableau_page<NoeudPousseContexte> m_noeuds_pousse_contexte{};
	tableau_page<NoeudExpressionAppel> m_noeuds_appel{};
	tableau_page<NoeudTableauArgsVariadiques> m_noeuds_tableau_args_variadiques{};
	tableau_page<NoeudTente> m_noeuds_tente{};
	tableau_page<NoeudDirectiveExecution> m_noeuds_directive_execution{};
	tableau_page<NoeudExpressionVirgule> m_noeuds_expression_virgule{};

	AllocatriceNoeud() = default;
	~AllocatriceNoeud() = default;

	NoeudExpression *cree_noeud(GenreNoeud genre);

	long nombre_noeuds() const;

	void rassemble_statistiques(Statistiques &stats) const;
};
