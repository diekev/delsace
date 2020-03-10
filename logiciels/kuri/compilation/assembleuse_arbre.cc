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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "assembleuse_arbre.h"

#include "contexte_generation_code.h"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
	: m_contexte(contexte)
{
	this->empile_bloc();

	/* Pour fprintf dans les messages d'erreurs, nous incluons toujours "stdio.h". */
	this->ajoute_inclusion("stdio.h");
	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	this->ajoute_inclusion("stdlib.h");
	/* Pour strlen, nous incluons toujours "string.h". */
	this->ajoute_inclusion("string.h");
	/* Pour les coroutines nous incluons toujours pthread */
	this->ajoute_inclusion("pthread.h");
	this->bibliotheques_dynamiques.pousse("pthread");
	this->definitions.pousse("_REENTRANT");
}

NoeudBloc *assembleuse_arbre::empile_bloc()
{
	auto bloc = static_cast<NoeudBloc *>(cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, nullptr));
	bloc->parent = bloc_courant();
	bloc->bloc_parent = bloc_courant();

	m_blocs.empile(bloc);

	return bloc;
}

NoeudBloc *assembleuse_arbre::bloc_courant() const
{
	if (m_blocs.est_vide()) {
		return nullptr;
	}

	return m_blocs.haut();
}

void assembleuse_arbre::depile_bloc()
{
	m_blocs.depile();
}

NoeudBase *assembleuse_arbre::cree_noeud(GenreNoeud genre, DonneesLexeme const *lexeme)
{
	auto noeud = static_cast<NoeudBase *>(nullptr);

	switch (genre) {
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			noeud = m_noeuds_bloc.ajoute_element();
			break;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			noeud = m_noeuds_declaration_fonction.ajoute_element();
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
		case GenreNoeud::EXPRESSION_INDICE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			noeud = m_noeuds_expression_binaire.ajoute_element();
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
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			noeud = m_noeuds_expression_unaire.ajoute_element();
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
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
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			noeud = m_noeuds_pousse_contexte.ajoute_element();
			break;
		}
	}

	if (noeud != nullptr) {
		noeud->genre = genre;
		noeud->lexeme = lexeme;
		noeud->bloc_parent = bloc_courant();

		if (noeud->lexeme && noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE) {
			noeud->ident = m_contexte.table_identifiants.identifiant_pour_chaine(noeud->lexeme->chaine);
		}

		if (genre == GenreNoeud::EXPRESSION_APPEL_FONCTION) {
			/* requis pour pouvoir renseigner le noms de arguments depuis
			 * l'analyse. */
			noeud->valeur_calculee = dls::liste<dls::vue_chaine_compacte>{};

			/* requis pour déterminer le module dans le noeud d'accès point
			 * À FAIRE : trouver mieux pour accéder à cette information */
			noeud->module_appel = noeud->lexeme->fichier;
		}
	}

	return noeud;
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	auto memoire = 0ul;

#define COMPTE_MEMOIRE(Type, Tableau) \
	memoire += static_cast<size_t>(Tableau.pages.taille()) * 128 * sizeof(Type)

	COMPTE_MEMOIRE(NoeudBloc, m_noeuds_bloc);
	COMPTE_MEMOIRE(NoeudDeclarationVariable, m_noeuds_declaration_variable);
	COMPTE_MEMOIRE(NoeudDeclarationFonction, m_noeuds_declaration_fonction);
	COMPTE_MEMOIRE(NoeudEnum, m_noeuds_enum);
	COMPTE_MEMOIRE(NoeudStruct, m_noeuds_struct);
	COMPTE_MEMOIRE(NoeudExpressionBinaire, m_noeuds_expression_binaire);
	COMPTE_MEMOIRE(NoeudExpressionAppel, m_noeuds_appel);
	COMPTE_MEMOIRE(NoeudExpressionLogement, m_noeuds_expression_logement);
	COMPTE_MEMOIRE(NoeudExpressionUnaire, m_noeuds_expression_unaire);
	COMPTE_MEMOIRE(NoeudExpression, m_noeuds_expression);
	COMPTE_MEMOIRE(NoeudBoucle, m_noeuds_boucle);
	COMPTE_MEMOIRE(NoeudPour, m_noeuds_pour);
	COMPTE_MEMOIRE(NoeudDiscr, m_noeuds_discr);
	COMPTE_MEMOIRE(NoeudSi, m_noeuds_si);
	COMPTE_MEMOIRE(NoeudPousseContexte, m_noeuds_pousse_contexte);
	COMPTE_MEMOIRE(NoeudTableauArgsVariadiques, m_noeuds_tableau_args_variadiques);

#undef COMPTE_MEMOIRE

	return memoire;
}

size_t assembleuse_arbre::nombre_noeuds() const
{
	auto noeuds = 0ul;

	noeuds += static_cast<size_t>(m_noeuds_bloc.taille());
	noeuds += static_cast<size_t>(m_noeuds_declaration_variable.taille());
	noeuds += static_cast<size_t>(m_noeuds_declaration_fonction.taille());
	noeuds += static_cast<size_t>(m_noeuds_enum.taille());
	noeuds += static_cast<size_t>(m_noeuds_struct.taille());
	noeuds += static_cast<size_t>(m_noeuds_expression_binaire.taille());
	noeuds += static_cast<size_t>(m_noeuds_appel.taille());
	noeuds += static_cast<size_t>(m_noeuds_expression_logement.taille());
	noeuds += static_cast<size_t>(m_noeuds_expression_unaire.taille());
	noeuds += static_cast<size_t>(m_noeuds_expression.taille());
	noeuds += static_cast<size_t>(m_noeuds_boucle.taille());
	noeuds += static_cast<size_t>(m_noeuds_pour.taille());
	noeuds += static_cast<size_t>(m_noeuds_discr.taille());
	noeuds += static_cast<size_t>(m_noeuds_si.taille());
	noeuds += static_cast<size_t>(m_noeuds_pousse_contexte.taille());
	noeuds += static_cast<size_t>(m_noeuds_tableau_args_variadiques.taille());

	return noeuds;
}

void assembleuse_arbre::ajoute_inclusion(const dls::chaine &fichier)
{
	if (deja_inclus.trouve(fichier) != deja_inclus.fin()) {
		return;
	}

	deja_inclus.insere(fichier);
	inclusions.pousse(fichier);
}
