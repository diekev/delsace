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

#include "arbre_syntaxique.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "identifiant.hh"
#include "modules.hh"
#include "outils_lexemes.hh"
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
	switch (genre) {
#define ENUMERE_GENRE_NOEUD_EX(genre) case GenreNoeud::genre: return #genre;
		ENUMERE_GENRES_NOEUD
#undef ENUMERE_GENRE_NOEUD_EX
	}
	return "erreur : GenreNoeud inconnu";
}

std::ostream &operator<<(std::ostream &os, GenreNoeud genre)
{
	os << chaine_genre_noeud(genre);
	return os;
}

bool est_expression_logique(NoeudExpression *noeud)
{
	if (noeud->genre == GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE) {
		return true;
	}

	if (noeud->lexeme->genre == GenreLexeme::BARRE_BARRE || noeud->lexeme->genre == GenreLexeme::ESP_ESP) {
		return true;
	}

	if (noeud->genre == GenreNoeud::EXPRESSION_PARENTHESE) {
		auto expr_paren = static_cast<NoeudExpressionUnaire *>(noeud);
		return est_expression_logique(expr_paren->expr);
	}

	return false;
}

/* ************************************************************************** */

void imprime_arbre(NoeudExpression *racine, std::ostream &os, int tab)
{
	if (racine == nullptr) {
		return;
	}

	switch (racine->genre) {
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			imprime_tab(os, tab);
			os << "bloc : " << bloc->expressions->taille << " expression(s)\n";

			POUR (*bloc->expressions.verrou_lecture()) {
				imprime_arbre(it, os, tab + 1);
			}

			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto expr = racine->comme_entete_fonction();

			imprime_tab(os, tab);
			os << "fonc : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			POUR (expr->params) {
				imprime_arbre(it, os, tab + 1);
			}

			imprime_arbre(expr->corps, os, tab + 1);

			break;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			auto expr = static_cast<NoeudDeclarationCorpsFonction *>(racine);
			imprime_arbre(expr->bloc, os, tab);
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
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();
			imprime_tab(os, tab);
			os << "comme : " << chaine_type(expr->type) << '\n';
			imprime_arbre(expr->expression, os, tab + 1);
			imprime_arbre(expr->expression_type, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();

			imprime_tab(os, tab);
			os << "assignation : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->variable, os, tab + 1);
			imprime_arbre(expr->expression, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
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
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre *>(racine);

			imprime_tab(os, tab);
			os << "expr membre : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->accede, os, tab + 1);
			imprime_arbre(expr->membre, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			imprime_tab(os, tab);
			os << "expr appel : " << expr->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(expr->appelee, os, tab + 1);

			POUR (expr->exprs) {
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
			imprime_arbre(expr->expr_taille, os, tab + 1);
			imprime_arbre(expr->bloc, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::INSTRUCTION_EMPL:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);

			imprime_tab(os, tab);
			os << "expr unaire : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		{
			auto inst = racine->comme_charge();
			imprime_tab(os, tab);
			os << "charge :\n";
			imprime_arbre(inst->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			auto inst = racine->comme_importe();
			imprime_tab(os, tab);
			os << "importe :\n";
			imprime_arbre(inst->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			imprime_tab(os, tab);
			os << "retour : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			imprime_tab(os, tab);
			os << "retiens : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto expr = static_cast<NoeudDirectiveExecution *>(racine);

			imprime_tab(os, tab);
			os << "dir exécution :\n";

			imprime_arbre(expr->expr, os, tab + 1);
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			imprime_tab(os, tab);
			os << "expr init_de\n";
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			imprime_tab(os, tab);
			os << "expr : " << racine->lexeme->chaine;

			if (racine->ident) {
				os << " (ident: " << racine->ident << ")";
			}

			os << '\n';
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
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = racine->comme_si_statique();
			imprime_tab(os, tab);
			os << "controle : #si\n";
			imprime_arbre(inst->condition, os, tab + 1);
			imprime_arbre(inst->bloc_si_vrai, os, tab + 1);
			imprime_arbre(inst->bloc_si_faux, os, tab + 1);
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
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto expr = racine->comme_virgule();

			imprime_tab(os, tab);
			os << "virgule :\n";

			POUR (expr->expressions) {
				imprime_arbre(it, os, tab + 1);
			}

			break;
		}
	}
}

NoeudExpression *copie_noeud(
		AssembleuseArbre *assem,
		const NoeudExpression *racine,
		NoeudBloc *bloc_parent)
{
	if (racine == nullptr) {
		return nullptr;
	}

	auto nracine = assem->cree_noeud(racine->genre, racine->lexeme);
	nracine->expression_type = copie_noeud(assem, racine->expression_type, bloc_parent);
	nracine->ident = racine->ident;
	nracine->type = racine->type;
	nracine->bloc_parent = bloc_parent;
	nracine->drapeaux = racine->drapeaux;
	nracine->drapeaux &= ~DECLARATION_FUT_VALIDEE;

	switch (racine->genre) {
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc const *>(racine);
			auto nbloc = static_cast<NoeudBloc *>(nracine);
			nbloc->membres->reserve(bloc->membres->taille);
			nbloc->expressions->reserve(bloc->expressions->taille);

			POUR (*bloc->expressions.verrou_lecture()) {
				auto nexpr = copie_noeud(assem, it, nbloc);
				nbloc->expressions->pousse(nexpr);

				if (nexpr->est_declaration()) {
					nbloc->membres->pousse(static_cast<NoeudDeclaration *>(nexpr));
				}
			}

			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto expr  = racine->comme_entete_fonction();
			auto nexpr = nracine->comme_entete_fonction();

			nexpr->params.reserve(expr->params.taille);
			nexpr->params_sorties.reserve(expr->params_sorties.taille);
			nexpr->arbre_aplatis.reserve(expr->arbre_aplatis.taille);
			nexpr->est_declaration_type = expr->est_declaration_type;

			POUR (expr->params) {
				auto copie = copie_noeud(assem, it, bloc_parent);
				nexpr->params.pousse(static_cast<NoeudDeclaration *>(copie));
				aplatis_arbre(copie, nexpr->arbre_aplatis, DrapeauxNoeud::AUCUN);
			}

			POUR (expr->params_sorties) {
				auto copie = copie_noeud(assem, it, bloc_parent);
				nexpr->params_sorties.pousse(static_cast<NoeudDeclaration *>(copie));
				aplatis_arbre(copie, nexpr->arbre_aplatis, DrapeauxNoeud::AUCUN);
			}

			/* copie le corps du noeud directement */
			{
				auto expr_corps = expr->corps;
				auto nexpr_corps = nexpr->corps;

				nexpr_corps->ident = expr_corps->ident;
				nexpr_corps->type = expr_corps->type;
				nexpr_corps->bloc_parent = bloc_parent;
				nexpr_corps->drapeaux = expr_corps->drapeaux;
				nexpr_corps->drapeaux &= ~DECLARATION_FUT_VALIDEE;

				nexpr_corps->arbre_aplatis.reserve(expr_corps->arbre_aplatis.taille);
				nexpr_corps->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr_corps->bloc, bloc_parent));

				aplatis_arbre(nexpr_corps->bloc, nexpr_corps->arbre_aplatis, DrapeauxNoeud::AUCUN);
			}

			break;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			/* assert faux car les noeuds de corps et d'entêtes sont alloués en même temps */
			assert_rappel(false, [&]() { std::cerr << "Tentative de copie d'un corps de fonction seul\n"; });
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum const *>(racine);
			auto ndecl = static_cast<NoeudEnum *>(nracine);

			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct const *>(racine);
			auto ndecl = static_cast<NoeudStruct *>(nracine);

			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable const *>(racine);
			auto nexpr = static_cast<NoeudDeclarationVariable *>(nracine);

			nexpr->valeur = copie_noeud(assem, expr->valeur, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->expression_type = nexpr->valeur->expression_type;

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();
			auto nexpr = nracine->comme_comme();
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->expression_type = copie_noeud(assem, expr->expression_type, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();
			auto nexpr = nracine->comme_assignation();

			nexpr->variable = copie_noeud(assem, expr->variable, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto expr = static_cast<NoeudExpressionBinaire const *>(racine);
			auto nexpr = static_cast<NoeudExpressionBinaire *>(nracine);

			nexpr->expr1 = copie_noeud(assem, expr->expr1, bloc_parent);
			nexpr->expr2 = copie_noeud(assem, expr->expr2, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre const *>(racine);
			auto nexpr = static_cast<NoeudExpressionMembre *>(nracine);

			nexpr->accede = copie_noeud(assem, expr->accede, bloc_parent);
			nexpr->membre = copie_noeud(assem, expr->membre, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel const *>(racine);
			auto nexpr = static_cast<NoeudExpressionAppel *>(nracine);

			nexpr->appelee = copie_noeud(assem, expr->appelee, bloc_parent);

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
			auto expr = static_cast<NoeudExpressionLogement const *>(racine);
			auto nexpr = static_cast<NoeudExpressionLogement *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->expr_taille = copie_noeud(assem, expr->expr_taille, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
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
		{
			auto expr = static_cast<NoeudExpressionUnaire const *>(racine);
			auto nexpr = static_cast<NoeudExpressionUnaire *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			auto ninst = nracine->comme_retour();
			ninst->expr = copie_noeud(assem, inst->expr, bloc_parent);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			auto ninst = nracine->comme_retiens();
			ninst->expr = copie_noeud(assem, inst->expr, bloc_parent);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto expr = static_cast<NoeudDirectiveExecution const *>(racine);
			auto nexpr = static_cast<NoeudDirectiveExecution *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->fonction = static_cast<NoeudDeclarationEnteteFonction *>(copie_noeud(assem, expr->fonction, bloc_parent));
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto expr = static_cast<NoeudTableauArgsVariadiques const *>(racine);
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
			auto expr = static_cast<NoeudBoucle const *>(racine);
			auto nexpr = static_cast<NoeudBoucle *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc, bloc_parent));
			nexpr->bloc->appartiens_a_boucle = nexpr;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour const *>(racine);
			auto nexpr = static_cast<NoeudPour *>(nracine);

			nexpr->variable = copie_noeud(assem, expr->variable, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));
			nexpr->bloc_sansarret = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc_sansarret, bloc_parent));
			nexpr->bloc_sinon = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc_sinon, bloc_parent));
			nexpr->bloc->appartiens_a_boucle = nexpr;
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr const *>(racine);
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
			auto expr = static_cast<NoeudSi const *>(racine);
			auto nexpr = static_cast<NoeudSi *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc_si_vrai = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc_si_vrai, bloc_parent));
			nexpr->bloc_si_faux = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc_si_faux, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = racine->comme_si_statique();
			auto ninst = nracine->comme_si_statique();
			ninst->condition = copie_noeud(assem, inst->condition, bloc_parent);
			ninst->bloc_si_vrai = static_cast<NoeudBloc	*>(copie_noeud(assem, inst->bloc_si_vrai, bloc_parent));
			ninst->bloc_si_faux = static_cast<NoeudBloc	*>(copie_noeud(assem, inst->bloc_si_faux, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte const *>(racine);
			auto nexpr = static_cast<NoeudPousseContexte *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente const *>(racine);
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
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto expr = racine->comme_virgule();
			auto nexpr = nracine->comme_virgule();

			nexpr->expressions.reserve(expr->expressions.taille);

			POUR (expr->expressions) {
				nexpr->expressions.pousse(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
	}

	return nracine;
}

void aplatis_arbre(
		NoeudExpression *racine,
		kuri::tableau<NoeudExpression *> &arbre_aplatis,
		DrapeauxNoeud drapeau)
{
	if (racine == nullptr) {
		return;
	}

	switch (racine->genre) {
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			POUR (*bloc->expressions.verrou_lecture()) {
				aplatis_arbre(it, arbre_aplatis, drapeau);
			}

			// Il nous faut le bloc pour savoir quoi différer
			arbre_aplatis.pousse(bloc);

			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
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

			// N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
			aplatis_arbre(expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
			aplatis_arbre(expr->expression_type, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->variable, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();
			expr->drapeaux |= drapeau;
			aplatis_arbre(expr->expression, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expression_type, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->expr1, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expr2, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr = static_cast<NoeudExpressionBinaire *>(racine);
			expr->drapeaux |= drapeau;

			if (est_assignation_composee(expr->lexeme->genre)) {
				drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			}

			aplatis_arbre(expr->expr1, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expr2, arbre_aplatis, drapeau);

			if (expr->lexeme->genre != GenreLexeme::VIRGULE) {
				arbre_aplatis.pousse(expr);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre *>(racine);
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->accede, arbre_aplatis, drapeau);
			expr->accede->aide_generation_code = EST_NOEUD_ACCES;
			// n'ajoute pas le membre, car la validation sémantique le considérera
			// comme une référence déclaration, ce qui soit clashera avec une variable
			// du même nom, soit résultera en une erreur de compilation
			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);
			expr->drapeaux |= drapeau;

			auto appelee = expr->appelee;

			if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
				// pour les expresssions de références de membre, puisqu'elles peuvent être des
				// expressions d'appels avec syntaxe uniforme, nous n'aplatissons que la branche
				// de l'accédée, la branche de membre pouvant être une fonction, ferait échouer la
				// validation sémantique
				auto ref_membre = static_cast<NoeudExpressionMembre *>(appelee);
				aplatis_arbre(ref_membre->accede, arbre_aplatis, drapeau);
			}
			else {
				aplatis_arbre(appelee, arbre_aplatis, drapeau);
			}

			POUR (expr->params) {
				if (it->est_assignation()) {
					// n'aplatis pas le nom du paramètre car cela clasherait avec une variable locale,
					// ou résulterait en une erreur de compilation « variable inconnue »
					auto expr_assing = it->comme_assignation();
					aplatis_arbre(expr_assing->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
				}
				else {
					aplatis_arbre(it, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
				}
			}

			arbre_aplatis.pousse(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(racine);
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->expr, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expr_taille, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expression_type, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis, drapeau);

			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::INSTRUCTION_EMPL:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);
			expr->drapeaux |= drapeau;
			aplatis_arbre(expr->expr, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			racine->drapeaux |= drapeau;
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			inst->drapeaux |= drapeau;
			drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			aplatis_arbre(inst->expr, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(inst);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			inst->drapeaux |= drapeau;
			drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			aplatis_arbre(inst->expr, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(inst);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto expr = static_cast<NoeudDirectiveExecution *>(racine);
			expr->drapeaux |= drapeau;

			if (expr->ident == ID::assert_ || expr->ident == ID::test) {
				drapeau |= DROITE_ASSIGNATION;
			}

			aplatis_arbre(expr->expr, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(expr);
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			arbre_aplatis.pousse(racine->expression_type);
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
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

			aplatis_arbre(expr->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);

			// n'ajoute pas la variable, sa déclaration n'a pas de type
			aplatis_arbre(expr->expression, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(expr);

			aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);
			aplatis_arbre(expr->bloc_sansarret, arbre_aplatis, DrapeauxNoeud::AUCUN);
			aplatis_arbre(expr->bloc_sinon, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr *>(racine);

			aplatis_arbre(expr->expr, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(expr);

			POUR (expr->paires_discr) {
				aplatis_arbre(it.second, arbre_aplatis, DrapeauxNoeud::AUCUN);
			}

			aplatis_arbre(expr->bloc_sinon, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi *>(racine);

			aplatis_arbre(expr->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
			aplatis_arbre(expr->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = racine->comme_si_statique();
			aplatis_arbre(inst->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.pousse(inst);
			aplatis_arbre(inst->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
			arbre_aplatis.pousse(inst); // insère une deuxième fois pour pouvoir sauter le code du bloc_si_faux si la condition évalue à « vrai »
			inst->index_bloc_si_faux = static_cast<int>(arbre_aplatis.taille - 1);
			aplatis_arbre(inst->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);
			inst->index_apres = static_cast<int>(arbre_aplatis.taille - 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);

			arbre_aplatis.pousse(expr->expr);
			arbre_aplatis.pousse(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(racine);

			if (inst->expr_piege) {
				drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			}

			aplatis_arbre(inst->expr_appel, arbre_aplatis, drapeau);
			arbre_aplatis.pousse(inst);
			aplatis_arbre(inst->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			arbre_aplatis.pousse(racine);
			break;
		}
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto expr = racine->comme_virgule();

			POUR (expr->expressions) {
				aplatis_arbre(it, arbre_aplatis, drapeau);
			}

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
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();

			auto etendue_enfant = calcule_etendue_noeud(expr->expression, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression_type, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();

			auto etendue_enfant = calcule_etendue_noeud(expr->variable, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
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
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->accede, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->membre, fichier);

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
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::INSTRUCTION_EMPL:
		case GenreNoeud::INSTRUCTION_CHARGE:
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->expr, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			auto etendue_enfant = calcule_etendue_noeud(inst->expr, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			auto etendue_enfant = calcule_etendue_noeud(inst->expr, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto dir = static_cast<NoeudDirectiveExecution *>(racine);
			auto etendue_enfant = calcule_etendue_noeud(dir->expr, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			POUR (expr->params) {
				auto etendue_enfant = calcule_etendue_noeud(it, fichier);

				etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
				etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			}

			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
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
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto expr = racine->comme_virgule();

			POUR (expr->expressions) {
				auto etendue_enfant = calcule_etendue_noeud(it, fichier);
				etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
				etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DECLARATION_STRUCTURE:
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
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
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		case GenreNoeud::INSTRUCTION_TENTE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
	}

	return etendue;
}

NoeudDeclarationVariable *NoeudDeclarationEnteteFonction::parametre_entree(long i) const
{
	auto param = params[i];

	if (param->est_empl()) {
		return param->comme_empl()->expr->comme_decl_var();
	}

	return param->comme_decl_var();
}
