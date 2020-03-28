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

#include "arbre_syntactic.h"

#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "modules.hh"
#include "typage.hh"

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

/* ************************************************************************** */

const char *chaine_genre_noeud(GenreNoeud genre)
{
#define CAS_GENRE(x) case x: return #x;

	switch (genre) {
		CAS_GENRE(GenreNoeud::RACINE)
		CAS_GENRE(GenreNoeud::DECLARATION_FONCTION)
		CAS_GENRE(GenreNoeud::DECLARATION_COROUTINE)
		CAS_GENRE(GenreNoeud::DECLARATION_OPERATEUR)
		CAS_GENRE(GenreNoeud::EXPRESSION_APPEL_FONCTION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE)
		CAS_GENRE(GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE)
		CAS_GENRE(GenreNoeud::DECLARATION_VARIABLE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER)
		CAS_GENRE(GenreNoeud::OPERATEUR_BINAIRE)
		CAS_GENRE(GenreNoeud::OPERATEUR_UNAIRE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR_SIMPLE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_CHAINE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_CARACTERE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SI)
		CAS_GENRE(GenreNoeud::INSTRUCTION_COMPOSEE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_POUR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_BOUCLE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_REPETE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_TANTQUE)
		CAS_GENRE(GenreNoeud::EXPRESSION_TRANSTYPE)
		CAS_GENRE(GenreNoeud::EXPRESSION_MEMOIRE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NUL)
		CAS_GENRE(GenreNoeud::EXPRESSION_TAILLE_DE)
		CAS_GENRE(GenreNoeud::EXPRESSION_PLAGE)
		CAS_GENRE(GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES)
		CAS_GENRE(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU)
		CAS_GENRE(GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE)
		CAS_GENRE(GenreNoeud::EXPRESSION_INFO_DE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LOGE)
		CAS_GENRE(GenreNoeud::EXPRESSION_DELOGE)
		CAS_GENRE(GenreNoeud::EXPRESSION_RELOGE)
		CAS_GENRE(GenreNoeud::DECLARATION_STRUCTURE)
		CAS_GENRE(GenreNoeud::DECLARATION_ENUM)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SAUFSI)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETIENS)
		CAS_GENRE(GenreNoeud::EXPRESSION_INDICE)
		CAS_GENRE(GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR_ENUM)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR_UNION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SINON)
		CAS_GENRE(GenreNoeud::EXPRESSION_PARENTHESE)
		CAS_GENRE(GenreNoeud::DIRECTIVE_EXECUTION)
		CAS_GENRE(GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE)
		CAS_GENRE(GenreNoeud::EXPANSION_VARIADIQUE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_TENTE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_NON_INITIALISATION)
	}

	return "erreur : GenreNoeud inconnu";
#undef CAS_GENRE
}

bool est_declaration(GenreNoeud genre)
{
	return dls::outils::est_element(
				genre,
				GenreNoeud::DECLARATION_VARIABLE,
				GenreNoeud::DECLARATION_FONCTION,
				GenreNoeud::DECLARATION_COROUTINE,
				GenreNoeud::DECLARATION_ENUM,
				GenreNoeud::DECLARATION_STRUCTURE);
}

/* ************************************************************************** */

void rassemble_feuilles(
		NoeudExpression *noeud_base,
		dls::tableau<NoeudExpression *> &feuilles)
{
	if (noeud_base == nullptr) {
		return;
	}

	if (noeud_base->lexeme->genre != GenreLexeme::VIRGULE) {
		feuilles.pousse(noeud_base);
		return;
	}

	auto expr = static_cast<NoeudExpressionBinaire *>(noeud_base);

	if (expr->expr1->lexeme->genre == GenreLexeme::VIRGULE) {
		rassemble_feuilles(expr->expr1, feuilles);
	}
	else {
		feuilles.pousse(expr->expr1);
	}

	if (expr->expr2->lexeme->genre == GenreLexeme::VIRGULE) {
		rassemble_feuilles(expr->expr2, feuilles);
	}
	else {
		feuilles.pousse(expr->expr2);
	}
}

void imprime_arbre(NoeudBase *racine, std::ostream &os, int tab)
{
	if (racine == nullptr) {
		return;
	}

	switch (racine->genre) {
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			imprime_tab(os, tab);
			os << "bloc : " << bloc->expressions.taille << " expression(s)\n";

			POUR (bloc->expressions) {
				imprime_arbre(it, os, tab + 1);
			}

			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			auto expr = static_cast<NoeudDeclarationFonction *>(racine);

			imprime_tab(os, tab);
			os << "fonc : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			POUR (expr->params) {
				imprime_arbre(it, os, tab + 1);
			}

			imprime_arbre(expr->bloc, os, tab + 1);

			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum *>(racine);

			imprime_tab(os, tab);
			os << "énum : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(decl->bloc, os, tab + 1);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(racine);

			imprime_tab(os, tab);
			os << "struct : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(decl->bloc, os, tab + 1);
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);

			imprime_tab(os, tab);
			os << "decl var : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->valeur, os, tab + 1);
			imprime_arbre(expr->expression, os, tab + 1);
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
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);

			imprime_tab(os, tab);
			os << "expr binaire : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->expr1, os, tab + 1);
			imprime_arbre(expr->expr2, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			imprime_tab(os, tab);
			os << "expr appel : " << expr->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			POUR (expr->params) {
				imprime_arbre(it, os, tab + 1);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(racine);

			imprime_tab(os, tab);
			os << "expr logement : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->expr, os, tab + 1);
			imprime_arbre(expr->expr_chaine, os, tab + 1);
			imprime_arbre(expr->bloc, os, tab + 1);
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
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);

			imprime_tab(os, tab);
			os << "expr unaire : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->expr, os, tab + 1);
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
			imprime_tab(os, tab);
			os << "expr : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto expr = static_cast<NoeudTableauArgsVariadiques *>(racine);

			imprime_tab(os, tab);
			os << "expr args variadiques : " << expr->exprs.taille << " expression(s)\n";

			POUR (expr->exprs) {
				imprime_arbre(it, os, tab + 1);
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto expr = static_cast<NoeudBoucle *>(racine);

			imprime_tab(os, tab);
			os << "boucle : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->condition, os, tab + 1);
			imprime_arbre(expr->bloc, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);

			imprime_tab(os, tab);
			os << "pour :\n'";

			imprime_arbre(expr->variable, os, tab + 1);
			imprime_arbre(expr->expression, os, tab + 1);
			imprime_arbre(expr->bloc, os, tab + 1);
			imprime_arbre(expr->bloc_sansarret, os, tab + 1);
			imprime_arbre(expr->bloc_sinon, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr *>(racine);

			imprime_tab(os, tab);
			os << "discr :\n'";

			imprime_arbre(expr->expr, os, tab + 1);

			POUR (expr->paires_discr) {
				imprime_arbre(it.first, os, tab + 1);
				imprime_arbre(it.second, os, tab + 1);
			}

			imprime_arbre(expr->bloc_sinon, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi *>(racine);

			imprime_tab(os, tab);
			os << "controle : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->condition, os, tab + 1);
			imprime_arbre(expr->bloc_si_vrai, os, tab + 1);
			imprime_arbre(expr->bloc_si_faux, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);

			os << "pousse_contexte :\n";

			imprime_arbre(expr->expr, os, tab + 1);
			imprime_arbre(expr->bloc, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(racine);

			os << "tente :\n";

			imprime_arbre(inst->expr_appel, os, tab + 1);
			imprime_arbre(inst->expr_piege, os, tab + 1);
			imprime_arbre(inst->bloc, os, tab + 1);

			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			imprime_tab(os, tab);
			os << "---";
			break;
		}
	}
}

NoeudExpression *copie_noeud(
		assembleuse_arbre *assem,
		NoeudExpression *racine,
		NoeudBloc *bloc_parent)
{
	if (racine == nullptr) {
		return nullptr;
	}

	auto nracine = assem->cree_noeud(racine->genre, racine->lexeme);
	nracine->type_declare = racine->type_declare;

	nracine->type_declare.expressions.efface();

	POUR (racine->type_declare.expressions) {
		nracine->type_declare.expressions.pousse(copie_noeud(assem, it, bloc_parent));
	}

	nracine->ident = racine->ident;
	nracine->type = racine->type;
	nracine->type_fonc = racine->type_fonc;
	nracine->bloc_parent = bloc_parent;
	nracine->drapeaux = racine->drapeaux;

	switch (racine->genre) {
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);
			auto nbloc = static_cast<NoeudBloc *>(nracine);
			nbloc->parent = bloc_parent;
			nbloc->membres.reserve(bloc->membres.taille);
			nbloc->expressions.reserve(bloc->expressions.taille);

			POUR (bloc->expressions) {
				auto nexpr = copie_noeud(assem, it, nbloc);
				nbloc->expressions.pousse(nexpr);

				if (est_declaration(nexpr->genre)) {
					nbloc->membres.pousse(static_cast<NoeudDeclaration *>(nexpr));
				}
			}

			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			auto expr = static_cast<NoeudDeclarationFonction *>(racine);
			auto nexpr = static_cast<NoeudDeclarationFonction *>(nracine);
			nexpr->params.reserve(expr->params.taille);
			nexpr->noms_retours.reserve(expr->noms_retours.taille);

			POUR (expr->params) {
				auto copie = copie_noeud(assem, it, bloc_parent);
				nexpr->params.pousse(static_cast<NoeudDeclaration *>(copie));
			}

			POUR (expr->noms_retours) {
				nexpr->noms_retours.pousse(it);
			}

			nexpr->drapeaux_decl = expr->drapeaux_decl;
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));

			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum *>(racine);
			auto ndecl = static_cast<NoeudEnum *>(nracine);

			ndecl->drapeaux_decl = decl->drapeaux_decl;
			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(racine);
			auto ndecl = static_cast<NoeudStruct *>(nracine);

			ndecl->drapeaux_decl = decl->drapeaux_decl;
			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);
			auto nexpr = static_cast<NoeudDeclarationVariable *>(nracine);

			nexpr->drapeaux_decl = expr->drapeaux_decl;
			nexpr->valeur = copie_noeud(assem, expr->valeur, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);

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
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);
			auto nexpr = static_cast<NoeudExpressionBinaire *>(nracine);

			nexpr->expr1 = copie_noeud(assem, expr->expr1, bloc_parent);
			nexpr->expr2 = copie_noeud(assem, expr->expr2, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);
			auto nexpr = static_cast<NoeudExpressionAppel *>(nracine);
			nexpr->params.reserve(expr->params.taille);

			POUR (expr->params) {
				nexpr->params.pousse(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(racine);
			auto nexpr = static_cast<NoeudExpressionLogement *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->expr_chaine = copie_noeud(assem, expr->expr_chaine, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));
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
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);
			auto nexpr = static_cast<NoeudExpressionUnaire *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
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
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto expr = static_cast<NoeudTableauArgsVariadiques *>(racine);
			auto nexpr = static_cast<NoeudTableauArgsVariadiques *>(nracine);
			nexpr->exprs.reserve(expr->exprs.taille);

			POUR (expr->exprs) {
				nexpr->exprs.pousse(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto expr = static_cast<NoeudBoucle *>(racine);
			auto nexpr = static_cast<NoeudBoucle *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);
			auto nexpr = static_cast<NoeudPour *>(nracine);

			nexpr->variable = copie_noeud(assem, expr->variable, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));
			nexpr->bloc_sansarret = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc_sansarret, bloc_parent));
			nexpr->bloc_sinon = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc_sinon, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr *>(racine);
			auto nexpr = static_cast<NoeudDiscr *>(nracine);
			nexpr->paires_discr.reserve(expr->paires_discr.taille);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->bloc_sinon = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc_sinon, bloc_parent));

			POUR (expr->paires_discr) {
				auto nexpr_paire = copie_noeud(assem, it.first, bloc_parent);
				auto nbloc_paire = static_cast<NoeudBloc *>(copie_noeud(assem, it.second, bloc_parent));

				nexpr->paires_discr.pousse({ nexpr_paire, nbloc_paire });
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi *>(racine);
			auto nexpr = static_cast<NoeudSi *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc_si_vrai = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc_si_vrai, bloc_parent));
			nexpr->bloc_si_faux = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc_si_faux, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);
			auto nexpr = static_cast<NoeudPousseContexte *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(racine);
			auto ninst = static_cast<NoeudTente *>(nracine);

			ninst->expr_appel = copie_noeud(assem, inst->expr_appel, bloc_parent);
			ninst->expr_piege = copie_noeud(assem, inst->expr_piege, bloc_parent);
			ninst->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, inst->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
	}

	return static_cast<NoeudExpression *>(nracine);
}

void aplatis_arbre(
		NoeudExpression *racine,
		kuri::tableau<NoeudExpression *> &arbre_aplatis)
{
	if (racine == nullptr) {
		return;
	}

	switch (racine->genre) {
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			POUR (bloc->expressions) {
				aplatis_arbre(it, arbre_aplatis);
			}

			// Il nous faut le bloc pour savoir quoi différer
			arbre_aplatis.pousse(bloc);

			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			/* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait */
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);

			aplatis_arbre(expr->expression, arbre_aplatis);
			arbre_aplatis.pousse(expr->valeur);
			arbre_aplatis.pousse(expr);

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
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);

			aplatis_arbre(expr->expr1, arbre_aplatis);
			aplatis_arbre(expr->expr2, arbre_aplatis);
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			POUR (expr->params) {
				aplatis_arbre(it, arbre_aplatis);
			}

			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(racine);

			aplatis_arbre(expr->expr, arbre_aplatis);
			aplatis_arbre(expr->expr_chaine, arbre_aplatis);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis);

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
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);
			aplatis_arbre(expr->expr, arbre_aplatis);
			arbre_aplatis.pousse(expr);
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
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			// Ceci ne doit pas être dans l'arbre à ce niveau
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto expr = static_cast<NoeudBoucle *>(racine);

			aplatis_arbre(expr->condition, arbre_aplatis);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);

			aplatis_arbre(expr->variable, arbre_aplatis);
			aplatis_arbre(expr->expression, arbre_aplatis);
			arbre_aplatis.pousse(expr);

			aplatis_arbre(expr->bloc, arbre_aplatis);
			aplatis_arbre(expr->bloc_sansarret, arbre_aplatis);
			aplatis_arbre(expr->bloc_sinon, arbre_aplatis);

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr *>(racine);

			// À FAIRE
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi *>(racine);

			aplatis_arbre(expr->condition, arbre_aplatis);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc_si_vrai, arbre_aplatis);
			aplatis_arbre(expr->bloc_si_faux, arbre_aplatis);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);

			arbre_aplatis.pousse(expr->expr);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis);

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(racine);

			aplatis_arbre(inst->expr_appel, arbre_aplatis);
			arbre_aplatis.pousse(inst);
			arbre_aplatis.pousse(inst->expr_piege);
			aplatis_arbre(inst->bloc, arbre_aplatis);

			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			arbre_aplatis.pousse(racine);
			break;
		}
	}
}

Etendue calcule_etendue_noeud(NoeudExpression *racine, Fichier *fichier)
{
	if (racine == nullptr) {
		return {};
	}

	auto const &lexeme = racine->lexeme;
	auto pos = position_lexeme(*lexeme);

	auto etendue = Etendue{};
	etendue.pos_min = pos.pos;
	etendue.pos_max = pos.pos + racine->lexeme->chaine.taille();

	switch (racine->genre) {
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->valeur, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

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
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->expr1, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expr2, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->expr, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->expr, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->expr, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			// prend en compte la parenthèse fermante
			etendue.pos_max += 1;

			break;
		}
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DECLARATION_STRUCTURE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		case GenreNoeud::INSTRUCTION_POUR:
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		case GenreNoeud::INSTRUCTION_TENTE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
	}

	return etendue;
}
