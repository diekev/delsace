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

#include "allocatrice_noeud.hh"

#include "biblinternes/outils/definitions.h"

NoeudExpression *AllocatriceNoeud::cree_noeud(GenreNoeud genre)
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
			assert(false);
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
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::EXPRESSION_COMME:
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
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			noeud = m_noeuds_expression_logement.ajoute_element();
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::INSTRUCTION_EMPL:
		{
			noeud = m_noeuds_expression_unaire.ajoute_element();
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			noeud = m_noeuds_directive_execution.ajoute_element();
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
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

long AllocatriceNoeud::memoire_utilisee() const
{
	auto memoire = 0l;

	memoire += m_noeuds_bloc.memoire_utilisee();
	memoire += m_noeuds_declaration_variable.memoire_utilisee();
	memoire += m_noeuds_declaration_corps_fonction.memoire_utilisee();
	memoire += m_noeuds_enum.memoire_utilisee();
	memoire += m_noeuds_struct.memoire_utilisee();
	memoire += m_noeuds_expression_binaire.memoire_utilisee();
	memoire += m_noeuds_expression_membre.memoire_utilisee();
	memoire += m_noeuds_expression_reference.memoire_utilisee();
	memoire += m_noeuds_appel.memoire_utilisee();
	memoire += m_noeuds_expression_logement.memoire_utilisee();
	memoire += m_noeuds_expression_unaire.memoire_utilisee();
	memoire += m_noeuds_expression.memoire_utilisee();
	memoire += m_noeuds_boucle.memoire_utilisee();
	memoire += m_noeuds_pour.memoire_utilisee();
	memoire += m_noeuds_discr.memoire_utilisee();
	memoire += m_noeuds_si.memoire_utilisee();
	memoire += m_noeuds_si_statique.memoire_utilisee();
	memoire += m_noeuds_pousse_contexte.memoire_utilisee();
	memoire += m_noeuds_tableau_args_variadiques.memoire_utilisee();
	memoire += m_noeuds_tente.memoire_utilisee();
	memoire += m_noeuds_directive_execution.memoire_utilisee();
	memoire += m_noeuds_expression_virgule.memoire_utilisee();

	pour_chaque_element(m_noeuds_struct, [&](NoeudStruct const &noeud)
	{
		memoire += noeud.arbre_aplatis.taille * taille_de(NoeudExpression *);
	});

	pour_chaque_element(m_noeuds_bloc, [&](NoeudBloc const &noeud)
	{
		memoire += noeud.membres->taille * taille_de(NoeudDeclaration *);
		memoire += noeud.expressions->taille * taille_de(NoeudExpression *);
		memoire += noeud.noeuds_differes.taille * taille_de(NoeudBloc *);
	});

	pour_chaque_element(m_noeuds_declaration_corps_fonction, [&](NoeudDeclarationCorpsFonction const &noeud)
	{
		memoire += noeud.arbre_aplatis.taille * taille_de(NoeudExpression *);
	});

	pour_chaque_element(m_noeuds_declaration_entete_fonction, [&](NoeudDeclarationEnteteFonction const &noeud)
	{
		memoire += noeud.params.taille * taille_de(NoeudDeclaration *);
		memoire += noeud.params_sorties.taille * taille_de(NoeudExpression *);
		memoire += noeud.arbre_aplatis.taille * taille_de(NoeudExpression *);
		memoire += noeud.noms_retours.taille * taille_de(dls::chaine);
		memoire += noeud.noms_types_gabarits.taille * taille_de(dls::vue_chaine_compacte);
		memoire += noeud.paires_expansion_gabarit.taille() * (taille_de (Type *) + taille_de (dls::vue_chaine_compacte));

		POUR (noeud.noms_retours) {
			memoire += it.taille();
		}

		memoire += noeud.epandu_pour.taille() * (taille_de(NoeudDeclarationEnteteFonction::tableau_paire_expansion) + taille_de(NoeudDeclarationCorpsFonction *));
		POUR (noeud.epandu_pour) {
			memoire += it.first.taille() * (taille_de (Type *) + taille_de (dls::vue_chaine_compacte));
		}

		memoire += noeud.nom_broye.taille();
	});

	pour_chaque_element(m_noeuds_appel, [&](NoeudExpressionAppel const &noeud)
	{
		memoire += noeud.params.taille * taille_de(NoeudExpression *);
		memoire += noeud.exprs.taille * taille_de(NoeudExpression *);
	});

	pour_chaque_element(m_noeuds_discr, [&](NoeudDiscr const &noeud)
	{
		using type_paire = std::pair<NoeudExpression *, NoeudBloc *>;
		memoire += noeud.paires_discr.taille * taille_de(type_paire);
	});

	pour_chaque_element(m_noeuds_tableau_args_variadiques, [&](NoeudTableauArgsVariadiques const &noeud)
	{
		memoire += noeud.exprs.taille * taille_de(NoeudExpression *);
	});

	pour_chaque_element(m_noeuds_expression_virgule, [&](NoeudExpressionVirgule const &noeud)
	{
		memoire += noeud.expressions.taille * taille_de(NoeudExpression *);
	});

#undef COMPTE_MEMOIRE

	return memoire;
}

long AllocatriceNoeud::nombre_noeuds() const
{
	auto noeuds = 0l;

	noeuds += m_noeuds_bloc.taille();
	noeuds += m_noeuds_declaration_variable.taille();
	noeuds += m_noeuds_declaration_corps_fonction.taille();
	noeuds += m_noeuds_declaration_entete_fonction.taille();
	noeuds += m_noeuds_enum.taille();
	noeuds += m_noeuds_struct.taille();
	noeuds += m_noeuds_expression_binaire.taille();
	noeuds += m_noeuds_expression_membre.taille();
	noeuds += m_noeuds_expression_reference.taille();
	noeuds += m_noeuds_appel.taille();
	noeuds += m_noeuds_expression_logement.taille();
	noeuds += m_noeuds_expression_unaire.taille();
	noeuds += m_noeuds_expression.taille();
	noeuds += m_noeuds_boucle.taille();
	noeuds += m_noeuds_pour.taille();
	noeuds += m_noeuds_discr.taille();
	noeuds += m_noeuds_si.taille();
	noeuds += m_noeuds_pousse_contexte.taille();
	noeuds += m_noeuds_tableau_args_variadiques.taille();
	noeuds += m_noeuds_directive_execution.taille();
	noeuds += m_noeuds_expression_virgule.taille();

#if 0
#define IMPRIME_NOMBRE_DE_NOEUDS(tableau) \
	std::cerr << "nombre de "#tableau" : " << tableau.taille() << '\n'

	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_bloc);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_declaration_variable);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_declaration_corps_fonction);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_declaration_entete_fonction);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_enum);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_struct);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_binaire);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_membre);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_reference);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_appel);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_logement);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_unaire);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_boucle);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_pour);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_discr);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_si);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_si_statique);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_pousse_contexte);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_tableau_args_variadiques);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_directive_execution);
	IMPRIME_NOMBRE_DE_NOEUDS(m_noeuds_expression_virgule);

#undef IMPRIME_NOMBRE_DE_NOEUDS
#endif

	return noeuds;
}
