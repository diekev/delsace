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
	tableau_page<NoeudComme> m_noeuds_comme{};
	tableau_page<NoeudDeclarationVariable> m_noeuds_declaration_variable{};
	tableau_page<NoeudExpressionReference> m_noeuds_expression_reference{};
	tableau_page<NoeudExpressionUnaire> m_noeuds_expression_unaire{};
	tableau_page<NoeudExpressionBinaire> m_noeuds_expression_binaire{};
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
	tableau_page<NoeudRetour> m_noeuds_retour{};
	tableau_page<NoeudAssignation> m_noeuds_assignation{};
	tableau_page<NoeudExpressionLitterale> m_noeuds_litterales{};

	AllocatriceNoeud() = default;
	~AllocatriceNoeud() = default;

	/* À FAIRE : supprime en faveur de la fonction ci-bas, uniquement utilisée pour les copies. */
	NoeudExpression *cree_noeud(GenreNoeud genre);

	/* Utilisation d'un gabarit car à part pour les copies, nous connaissons
	 * toujours le genre de noeud à créer, et spécialiser cette fonction nous
	 * économise pas mal de temps d'exécution, au prix d'un exécutable plus gros. */
	template <GenreNoeud genre>
	NoeudExpression *cree_noeud()
	{
		auto noeud = NoeudExpression::nul();

		switch (genre) {
			case GenreNoeud::INSTRUCTION_COMPOSEE:
			{
				noeud = m_noeuds_bloc.ajoute_element();
				break;
			}
			case GenreNoeud::DECLARATION_ENTETE_FONCTION:
			{
				auto entete = m_noeuds_declaration_entete_fonction.ajoute_element();
				auto corps  = m_noeuds_declaration_corps_fonction.ajoute_element();

				entete->corps = corps;
				corps->entete = entete;

				noeud = entete;
				break;
			}
			case GenreNoeud::DECLARATION_CORPS_FONCTION:
			{
				/* assert faux car les noeuds de corps et d'entêtes sont alloués en même temps */
				//static_assert(false, "Tentative d'allocation d'un corps de fonction seul");
				break;
			}
			case GenreNoeud::DECLARATION_ENUM:
			{
				noeud = m_noeuds_enum.ajoute_element();
				break;
			}
			case GenreNoeud::DECLARATION_STRUCTURE:
			{
				noeud = m_noeuds_struct.ajoute_element();
				break;
			}
			case GenreNoeud::DECLARATION_VARIABLE:
			{
				noeud = m_noeuds_declaration_variable.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_COMME:
			{
				noeud = m_noeuds_comme.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
			{
				noeud = m_noeuds_assignation.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_INDEXAGE:
			case GenreNoeud::EXPRESSION_PLAGE:
			case GenreNoeud::OPERATEUR_BINAIRE:
			case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
			{
				noeud = m_noeuds_expression_binaire.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
			case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
			{
				noeud = m_noeuds_expression_membre.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
			{
				noeud = m_noeuds_expression_reference.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
			case GenreNoeud::EXPRESSION_APPEL_FONCTION:
			{
				noeud = m_noeuds_appel.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
			case GenreNoeud::EXPRESSION_INFO_DE:
			case GenreNoeud::EXPRESSION_INIT_DE:
			case GenreNoeud::EXPRESSION_MEMOIRE:
			case GenreNoeud::EXPRESSION_PARENTHESE:
			case GenreNoeud::OPERATEUR_UNAIRE:
			case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
			case GenreNoeud::EXPRESSION_TAILLE_DE:
			case GenreNoeud::EXPANSION_VARIADIQUE:
			case GenreNoeud::EXPRESSION_TYPE_DE:
			case GenreNoeud::INSTRUCTION_EMPL:
			case GenreNoeud::INSTRUCTION_CHARGE:
			case GenreNoeud::INSTRUCTION_IMPORTE:
			case GenreNoeud::DIRECTIVE_CUISINE:
			{
				noeud = m_noeuds_expression_unaire.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_RETOUR:
			case GenreNoeud::INSTRUCTION_RETIENS:
			{
				noeud = m_noeuds_retour.ajoute_element();
				break;
			}
			case GenreNoeud::DIRECTIVE_EXECUTION:
			{
				noeud = m_noeuds_directive_execution.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
			case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
			case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
			case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
			case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
			case GenreNoeud::EXPRESSION_LITTERALE_NUL:
			{
				noeud = m_noeuds_litterales.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
			case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
			{
				noeud = m_noeuds_expression.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
			{
				noeud = m_noeuds_tableau_args_variadiques.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_BOUCLE:
			case GenreNoeud::INSTRUCTION_REPETE:
			case GenreNoeud::INSTRUCTION_TANTQUE:
			{
				noeud = m_noeuds_boucle.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_POUR:
			{
				noeud = m_noeuds_pour.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_DISCR:
			case GenreNoeud::INSTRUCTION_DISCR_ENUM:
			case GenreNoeud::INSTRUCTION_DISCR_UNION:
			{
				noeud = m_noeuds_discr.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_SAUFSI:
			case GenreNoeud::INSTRUCTION_SI:
			{
				noeud = m_noeuds_si.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_SI_STATIQUE:
			{
				noeud = m_noeuds_si_statique.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
			{
				noeud = m_noeuds_pousse_contexte.ajoute_element();
				break;
			}
			case GenreNoeud::INSTRUCTION_TENTE:
			{
				noeud = m_noeuds_tente.ajoute_element();
				break;
			}
			case GenreNoeud::EXPRESSION_VIRGULE:
			{
				noeud = m_noeuds_expression_virgule.ajoute_element();
				break;
			}
		}

		return noeud;
	}

	long nombre_noeuds() const;

	void rassemble_statistiques(Statistiques &stats) const;
};
