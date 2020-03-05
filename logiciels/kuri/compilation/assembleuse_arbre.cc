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

assembleuse_arbre::~assembleuse_arbre()
{
	/* NOTE : nous devons appeler les destructeurs pour libérer automatiquement
	 * la mémoire allouée dans les noeuds (chaines, tableaux, etc.) */

#define DELOGE_NOEUDS(Type, Tableau) \
	for (auto ptr : Tableau) {\
		memoire::deloge(#Type, ptr); \
	}

	DELOGE_NOEUDS(NoeudBloc, m_noeuds_bloc);
	DELOGE_NOEUDS(NoeudDeclarationVariable, m_noeuds_declaration_variable);
	DELOGE_NOEUDS(NoeudDeclarationFonction, m_noeuds_declaration_fonction);
	DELOGE_NOEUDS(NoeudEnum, m_noeuds_enum);
	DELOGE_NOEUDS(NoeudStruct, m_noeuds_struct);
	DELOGE_NOEUDS(NoeudExpressionBinaire, m_noeuds_expression_binaire);
	DELOGE_NOEUDS(NoeudExpressionAppel, m_noeuds_appel);
	DELOGE_NOEUDS(NoeudExpressionLogement, m_noeuds_expression_logement);
	DELOGE_NOEUDS(NoeudExpressionUnaire, m_noeuds_expression_unaire);
	DELOGE_NOEUDS(NoeudExpression, m_noeuds_expression);
	DELOGE_NOEUDS(NoeudBoucle, m_noeuds_boucle);
	DELOGE_NOEUDS(NoeudPour, m_noeuds_pour);
	DELOGE_NOEUDS(NoeudDiscr, m_noeuds_discr);
	DELOGE_NOEUDS(NoeudSi, m_noeuds_si);
	DELOGE_NOEUDS(NoeudPousseContexte, m_noeuds_pousse_contexte);
	DELOGE_NOEUDS(NoeudTableauArgsVariadiques, m_noeuds_tableau_args_variadiques);

#undef DELOGE_NOEUDS
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

#define LOGE_NOEUD(Type, Tableau) \
	auto ptr = memoire::loge<Type>(#Type); \
	Tableau.pousse(ptr); \
	noeud = ptr;

	switch (genre) {
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			LOGE_NOEUD(NoeudBloc, m_noeuds_bloc);
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			LOGE_NOEUD(NoeudDeclarationFonction, m_noeuds_declaration_fonction);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			LOGE_NOEUD(NoeudEnum, m_noeuds_enum);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			LOGE_NOEUD(NoeudStruct, m_noeuds_struct);
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			LOGE_NOEUD(NoeudDeclarationVariable, m_noeuds_declaration_variable);
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
			LOGE_NOEUD(NoeudExpressionBinaire, m_noeuds_expression_binaire);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			LOGE_NOEUD(NoeudExpressionAppel, m_noeuds_appel);
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			LOGE_NOEUD(NoeudExpressionLogement, m_noeuds_expression_logement);
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
			LOGE_NOEUD(NoeudExpressionUnaire, m_noeuds_expression_unaire);
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
			LOGE_NOEUD(NoeudExpression, m_noeuds_expression);
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			LOGE_NOEUD(NoeudTableauArgsVariadiques, m_noeuds_tableau_args_variadiques);
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			LOGE_NOEUD(NoeudBoucle, m_noeuds_boucle);
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			LOGE_NOEUD(NoeudPour, m_noeuds_pour);
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			LOGE_NOEUD(NoeudDiscr, m_noeuds_discr);
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			LOGE_NOEUD(NoeudSi, m_noeuds_si);
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			LOGE_NOEUD(NoeudPousseContexte, m_noeuds_pousse_contexte);
			break;
		}
	}

#undef LOGE_NOEUD

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
	memoire += static_cast<size_t>(Tableau.taille()) * (sizeof(Type *) + sizeof(Type))

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

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "DonneesLexeme          : " << sizeof(DonneesLexeme) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
