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
#include "broyage.hh"
#include "compilatrice.hh"
#include "erreur.h"
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

/* ************************************************************************** */

void imprime_arbre_substitue(NoeudExpression *racine, std::ostream &os, int tab)
{
	imprime_arbre(racine, os, tab, true);
}

void imprime_arbre(NoeudExpression *racine, std::ostream &os, int tab, bool substitution)
{
	if (racine == nullptr) {
		return;
	}

	if (substitution && racine->substitution) {
		imprime_arbre(racine->substitution, os, tab, substitution);
		return;
	}

	switch (racine->genre) {
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			imprime_tab(os, tab);
			os << "bloc : " << bloc->expressions->taille << " expression(s)\n";

			POUR (*bloc->expressions.verrou_lecture()) {
				imprime_arbre(it, os, tab + 1, substitution);
			}

			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto expr = racine->comme_entete_fonction();

			imprime_tab(os, tab);
			os << "fonc : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			POUR (expr->params) {
				imprime_arbre(it, os, tab + 1, substitution);
			}

			imprime_arbre(expr->corps, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			auto expr = static_cast<NoeudDeclarationCorpsFonction *>(racine);
			imprime_arbre(expr->bloc, os, tab, substitution);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			auto decl = static_cast<NoeudEnum *>(racine);

			imprime_tab(os, tab);
			os << "énum : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(decl->bloc, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct *>(racine);

			imprime_tab(os, tab);
			os << "struct : " << racine->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(decl->bloc, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);

			imprime_tab(os, tab);
			os << "decl var : " << (expr->ident ? expr->ident->nom : expr->lexeme->chaine) << '\n';

			imprime_arbre(expr->valeur, os, tab + 1, substitution);
			imprime_arbre(expr->expression, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();
			imprime_tab(os, tab);
			os << "comme : " << chaine_type(expr->type) << '\n';
			imprime_arbre(expr->expression, os, tab + 1, substitution);
			imprime_arbre(expr->expression_type, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();

			imprime_tab(os, tab);
			os << "assignation : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->variable, os, tab + 1, substitution);
			imprime_arbre(expr->expression, os, tab + 1, substitution);
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

			imprime_arbre(expr->expr1, os, tab + 1, substitution);
			imprime_arbre(expr->expr2, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre *>(racine);

			imprime_tab(os, tab);
			os << "expr membre : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->accede, os, tab + 1, substitution);
			imprime_arbre(expr->membre, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			imprime_tab(os, tab);
			os << "expr appel : " << expr->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(expr->appelee, os, tab + 1, substitution);

			POUR (expr->exprs) {
				imprime_arbre(it, os, tab + 1, substitution);
			}

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

			imprime_arbre(expr->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		{
			auto inst = racine->comme_charge();
			imprime_tab(os, tab);
			os << "charge :\n";
			imprime_arbre(inst->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			auto inst = racine->comme_importe();
			imprime_tab(os, tab);
			os << "importe :\n";
			imprime_arbre(inst->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			imprime_tab(os, tab);
			os << "retour : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			imprime_tab(os, tab);
			os << "retiens : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			auto cuisine = racine->comme_cuisine();
			imprime_tab(os, tab);
			os << "cuisine :\n";
			imprime_arbre(cuisine->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto expr = static_cast<NoeudDirectiveExecution *>(racine);

			imprime_tab(os, tab);
			os << "dir exécution :\n";

			imprime_arbre(expr->expr, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			imprime_tab(os, tab);
			os << "expr init_de\n";

			auto init_de = racine->comme_init_de();
			imprime_arbre(init_de->expr, os, tab + 1, substitution);

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
				imprime_arbre(it, os, tab + 1, substitution);
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

			imprime_arbre(expr->bloc_pre, os, tab + 1, substitution);
			imprime_arbre(expr->condition, os, tab + 1, substitution);
			imprime_arbre(expr->bloc, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_inc, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_sansarret, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_sinon, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);

			imprime_tab(os, tab);
			os << "pour :\n'";

			imprime_arbre(expr->variable, os, tab + 1, substitution);
			imprime_arbre(expr->expression, os, tab + 1, substitution);
			imprime_arbre(expr->bloc, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_sansarret, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_sinon, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto expr = static_cast<NoeudDiscr *>(racine);

			imprime_tab(os, tab);
			os << "discr :\n'";

			imprime_arbre(expr->expr, os, tab + 1, substitution);

			POUR (expr->paires_discr) {
				imprime_arbre(it.first, os, tab + 1, substitution);
				imprime_arbre(it.second, os, tab + 1, substitution);
			}

			imprime_arbre(expr->bloc_sinon, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi *>(racine);

			imprime_tab(os, tab);
			os << "controle : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->condition, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_si_vrai, os, tab + 1, substitution);
			imprime_arbre(expr->bloc_si_faux, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = racine->comme_si_statique();
			imprime_tab(os, tab);
			os << "controle : #si\n";
			imprime_arbre(inst->condition, os, tab + 1, substitution);
			imprime_arbre(inst->bloc_si_vrai, os, tab + 1, substitution);
			imprime_arbre(inst->bloc_si_faux, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);

			os << "pousse_contexte :\n";

			imprime_arbre(expr->expr, os, tab + 1, substitution);
			imprime_arbre(expr->bloc, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudTente *>(racine);

			os << "tente :\n";

			imprime_arbre(inst->expr_appel, os, tab + 1, substitution);
			imprime_arbre(inst->expr_piege, os, tab + 1, substitution);
			imprime_arbre(inst->bloc, os, tab + 1, substitution);

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
				imprime_arbre(it, os, tab + 1, substitution);
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
			nbloc->possede_contexte = bloc->possede_contexte;
			nbloc->est_differe = bloc->est_differe;

			POUR (*bloc->expressions.verrou_lecture()) {
				auto nexpr = copie_noeud(assem, it, nbloc);
				nbloc->expressions->ajoute(nexpr);

				if (nexpr->est_declaration()) {
					nbloc->membres->ajoute(static_cast<NoeudDeclaration *>(nexpr));
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

			nexpr->bloc_constantes = assem->cree_bloc_seul(nullptr, bloc_parent);
			nexpr->bloc_parametres = assem->cree_bloc_seul(nullptr, nexpr->bloc_constantes);

			bloc_parent = nexpr->bloc_parametres;

			POUR (expr->params) {
				auto copie = copie_noeud(assem, it, bloc_parent);
				nexpr->params.ajoute(static_cast<NoeudDeclarationVariable *>(copie));
			}

			POUR (expr->params_sorties) {
				auto copie = copie_noeud(assem, it, bloc_parent);
				nexpr->params_sorties.ajoute(static_cast<NoeudDeclarationVariable *>(copie));
			}

			if (expr->params_sorties.taille > 1) {
				nexpr->param_sortie = copie_noeud(assem, expr->param_sortie, bloc_parent)->comme_decl_var();
			}
			else {
				nexpr->param_sortie = nexpr->params_sorties[0];
			}

			nexpr->annotations.reserve(expr->annotations.taille());
			POUR (expr->annotations) {
				nexpr->annotations.ajoute(it);
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
				nexpr_corps->est_corps_texte = expr_corps->est_corps_texte;

				nexpr_corps->arbre_aplatis.reserve(expr_corps->arbre_aplatis.taille);
				nexpr_corps->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr_corps->bloc, bloc_parent));
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
			ndecl->expression_type = copie_noeud(assem, decl->expression_type, bloc_parent);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto decl = static_cast<NoeudStruct const *>(racine);
			auto ndecl = static_cast<NoeudStruct *>(nracine);

			ndecl->est_union = decl->est_union;
			ndecl->est_nonsure = decl->est_nonsure;
			ndecl->est_externe = decl->est_externe;
			ndecl->est_corps_texte = decl->est_corps_texte;

			if (decl->bloc_constantes) {
				ndecl->bloc_constantes = copie_noeud(assem, decl->bloc_constantes, bloc_parent)->comme_bloc();
				bloc_parent = ndecl->bloc_constantes;
			}

			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable const *>(racine);
			auto nexpr = static_cast<NoeudDeclarationVariable *>(nracine);

			nexpr->valeur = copie_noeud(assem, expr->valeur, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->expression_type = copie_noeud(assem, expr->expression_type, bloc_parent);

			/* n'oublions pas de mettre en place les déclarations */
			if (nexpr->valeur->est_ref_decl()) {
				nexpr->valeur->comme_ref_decl()->decl = nexpr;
			}
			else if (nexpr->valeur->est_virgule()) {
				auto virgule = expr->valeur->comme_virgule();
				auto nvirgule = nexpr->valeur->comme_virgule();

				auto index = 0;
				POUR (virgule->expressions) {
					auto it_orig = nvirgule->expressions[index]->comme_ref_decl();
					it->comme_ref_decl()->decl = copie_noeud(assem, it_orig->decl, bloc_parent)->comme_decl_var();
				}
			}

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
				nexpr->params.ajoute(copie_noeud(assem, it, bloc_parent));
			}

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
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			auto cuisine = racine->comme_cuisine();
			auto ncuisine = nracine->comme_cuisine();
			ncuisine->expr = copie_noeud(assem, cuisine->expr, bloc_parent);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			auto expr = static_cast<NoeudDirectiveExecution const *>(racine);
			auto nexpr = static_cast<NoeudDirectiveExecution *>(nracine);

			nexpr->expr = copie_noeud(assem, expr->expr, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			auto lit = racine->comme_litterale();
			auto nlit = nracine->comme_litterale();
			nlit->valeur_entiere = lit->valeur_entiere;
			break;
		}
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
				nexpr->exprs.ajoute(copie_noeud(assem, it, bloc_parent));
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

				nexpr->paires_discr.ajoute({ nexpr_paire, nbloc_paire });
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto expr = static_cast<NoeudSi const *>(racine);
			auto nexpr = static_cast<NoeudSi *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc_si_vrai = copie_noeud(assem, expr->bloc_si_vrai, bloc_parent);
			nexpr->bloc_si_faux = copie_noeud(assem, expr->bloc_si_faux, bloc_parent);
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
				nexpr->expressions.ajoute(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
	}

	return nracine;
}

static void aplatis_arbre(
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
			arbre_aplatis.ajoute(bloc);

			break;
		}
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			/* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait */
			arbre_aplatis.ajoute(racine);
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			arbre_aplatis.ajoute(racine);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			arbre_aplatis.ajoute(racine);
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = static_cast<NoeudDeclarationVariable *>(racine);

			// N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
			// Évite également les déclaration de types polymorphiques, cela gène la validation car la déclaration n'est dans aucun bloc.
			if (!expr->possede_drapeau(EST_DECLARATION_TYPE_OPAQUE) || !expr->expression->possede_drapeau(DECLARATION_TYPE_POLYMORPHIQUE)) {
				aplatis_arbre(expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
				aplatis_arbre(expr->expression_type, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
			}

			arbre_aplatis.ajoute(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation();
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->variable, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.ajoute(expr);

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();
			expr->drapeaux |= drapeau;
			aplatis_arbre(expr->expression, arbre_aplatis, drapeau);
			aplatis_arbre(expr->expression_type, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.ajoute(expr);
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
			arbre_aplatis.ajoute(expr);

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
				arbre_aplatis.ajoute(expr);
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
			arbre_aplatis.ajoute(expr);

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

			arbre_aplatis.ajoute(expr);

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
			arbre_aplatis.ajoute(expr);
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			racine->drapeaux |= drapeau;
			arbre_aplatis.ajoute(racine);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retour();
			inst->drapeaux |= drapeau;
			drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			aplatis_arbre(inst->expr, arbre_aplatis, drapeau);
			arbre_aplatis.ajoute(inst);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			inst->drapeaux |= drapeau;
			drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
			aplatis_arbre(inst->expr, arbre_aplatis, drapeau);
			arbre_aplatis.ajoute(inst);
			break;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			auto cuisine = racine->comme_cuisine();
			cuisine->drapeaux |= drapeau;

			drapeau |= DROITE_ASSIGNATION;
			drapeau |= POUR_CUISSON;

			aplatis_arbre(cuisine->expr, arbre_aplatis, drapeau);
			arbre_aplatis.ajoute(cuisine);
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
			arbre_aplatis.ajoute(expr);
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			auto init_de = racine->comme_init_de();
			aplatis_arbre(init_de->expr, arbre_aplatis, drapeau);
			arbre_aplatis.ajoute(racine);
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
			arbre_aplatis.ajoute(racine);
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

			aplatis_arbre(expr->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
			arbre_aplatis.ajoute(expr);
			aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto expr = static_cast<NoeudPour *>(racine);

			// n'ajoute pas la variable, sa déclaration n'a pas de type
			aplatis_arbre(expr->expression, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.ajoute(expr);

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
			arbre_aplatis.ajoute(expr);

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

			/* préserve le drapeau au cas où nous serions à droite d'une expression */
			expr->drapeaux |= drapeau;

			aplatis_arbre(expr->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
			aplatis_arbre(expr->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
			aplatis_arbre(expr->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);

			/* mets l'instruction à la fin afin de pouvoir déterminer le type de
			 * l'expression selon les blocs si nous sommes à droite d'une expression */
			arbre_aplatis.ajoute(expr);

			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = racine->comme_si_statique();
			aplatis_arbre(inst->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
			arbre_aplatis.ajoute(inst);
			aplatis_arbre(inst->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
			arbre_aplatis.ajoute(inst); // insère une deuxième fois pour pouvoir sauter le code du bloc_si_faux si la condition évalue à « vrai »
			inst->index_bloc_si_faux = static_cast<int>(arbre_aplatis.taille - 1);
			aplatis_arbre(inst->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);
			inst->index_apres = static_cast<int>(arbre_aplatis.taille - 1);
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto expr = static_cast<NoeudPousseContexte *>(racine);

			arbre_aplatis.ajoute(expr->expr);
			arbre_aplatis.ajoute(expr);
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
			arbre_aplatis.ajoute(inst);
			aplatis_arbre(inst->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			arbre_aplatis.ajoute(racine);
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

void aplatis_arbre(NoeudExpression *declaration)
{
	if (declaration->est_entete_fonction()) {
		auto entete = declaration->comme_entete_fonction();
		if (entete->arbre_aplatis.taille == 0) {
			aplatis_arbre(entete->bloc_constantes, entete->arbre_aplatis, {});
			aplatis_arbre(entete->bloc_parametres, entete->arbre_aplatis, {});

			POUR (entete->params) {
				aplatis_arbre(it, entete->arbre_aplatis, {});
			}

			POUR (entete->params_sorties) {
				aplatis_arbre(it, entete->arbre_aplatis, {});
			}
		}
		return;
	}

	if (declaration->est_corps_fonction()) {
		auto corps = declaration->comme_corps_fonction();
		if (corps->arbre_aplatis.taille == 0) {
			aplatis_arbre(corps->bloc, corps->arbre_aplatis, {});
		}
		return;
	}

	if (declaration->est_structure()) {
		auto structure = declaration->comme_structure();
		if (structure->arbre_aplatis.taille == 0) {
			POUR (structure->params_polymorphiques) {
				aplatis_arbre(it, structure->arbre_aplatis_params, {});
			}

			aplatis_arbre(structure->bloc, structure->arbre_aplatis, {});
		}
		return;
	}

	if (declaration->est_execute()) {
		auto execute = declaration->comme_execute();
		if (execute->arbre_aplatis.taille == 0) {
			aplatis_arbre(execute, execute->arbre_aplatis, {});
		}
		return;
	}

	if (declaration->est_decl_var()) {
		auto decl_var = declaration->comme_decl_var();
		if (decl_var->arbre_aplatis.taille == 0) {
			aplatis_arbre(decl_var, decl_var->arbre_aplatis, {});
		}
		return;
	}
}

Etendue calcule_etendue_noeud(const NoeudExpression *racine, Fichier *fichier)
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
			auto expr = racine->comme_decl_var();

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
			auto expr = static_cast<NoeudExpressionBinaire const *>(racine);

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
			auto expr = static_cast<NoeudExpressionMembre const *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->accede, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->membre, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
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
			auto expr = static_cast<NoeudExpressionUnaire const *>(racine);

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
			auto dir = static_cast<NoeudDirectiveExecution const *>(racine);
			auto etendue_enfant = calcule_etendue_noeud(dir->expr, fichier);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr = static_cast<NoeudExpressionAppel const *>(racine);

			auto etendue_expr = calcule_etendue_noeud(expr->appelee, fichier);

			etendue.pos_min = std::min(etendue.pos_min, etendue_expr.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_expr.pos_max);

			POUR (expr->params) {
				auto etendue_enfant = calcule_etendue_noeud(it, fichier);

				etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
				etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			}

			break;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionUnaire const *>(racine);

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

struct Simplificatrice {
	EspaceDeTravail *espace;
	AssembleuseArbre *assem;
	Typeuse &typeuse;

	NoeudDeclarationEnteteFonction *fonction_courante = nullptr;

	Simplificatrice(EspaceDeTravail *e, AssembleuseArbre *a, Typeuse &t)
		: espace(e)
		, assem(a)
		, typeuse(t)
	{}

	void simplifie(NoeudExpression *noeud);

private:
	void simplifie_boucle_pour(NoeudPour *inst);
	void simplifie_comparaison_chainee(NoeudExpressionBinaire *comp);
	void simplifie_coroutine(NoeudDeclarationEnteteFonction *corout);
	void simplifie_discr(NoeudDiscr *discr);
	template<int N>
	void simplifie_discr_impl(NoeudDiscr *discr);
	void simplifie_retiens(NoeudRetour *retiens);
	void simplifie_retour(NoeudRetour *inst);

	NoeudExpression *simplifie_operateur_binaire(NoeudExpressionBinaire *expr_bin, bool pour_operande);
	NoeudSi *cree_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud);
	NoeudExpression *cree_expression_pour_op_chainee(kuri::tableau<NoeudExpressionBinaire> &comparaisons, const Lexeme *lexeme_op_logique);

	/* remplace la dernière expression d'un bloc par une assignation afin de pouvoir simplifier les conditions à droite des assigations */
	void corrige_bloc_pour_assignation(NoeudExpression *expr, NoeudExpression *ref_temp);
};

void Simplificatrice::simplifie(NoeudExpression *noeud)
{
	if (!noeud) {
		return;
	}

	switch (noeud->genre) {
		case GenreNoeud::DECLARATION_ENTETE_FONCTION:
		{
			auto entete = noeud->comme_entete_fonction();

			if (entete->est_externe) {
				return;
			}

			if (entete->est_declaration_type) {
				entete->substitution = assem->cree_ref_type(entete->lexeme, entete->type);
				return;
			}

			if (entete->est_coroutine) {
				simplifie_coroutine(entete);
				return;
			}

			fonction_courante = entete;
			simplifie(entete->corps);
			return;
		}
		case GenreNoeud::DECLARATION_CORPS_FONCTION:
		{
			auto corps = noeud->comme_corps_fonction();
			simplifie(corps->bloc);
			return;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = noeud->comme_bloc();

			POUR (*bloc->expressions.verrou_ecriture()) {
				simplifie(it);
			}

			return;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr_bin = noeud->comme_operateur_binaire();

			if (expr_bin->type->est_type_de_donnees()) {
				noeud->substitution = assem->cree_ref_type(expr_bin->lexeme, expr_bin->type);
				return;
			}

			simplifie(expr_bin->expr1);
			simplifie(expr_bin->expr2);

			if (expr_bin->op && expr_bin->op->est_arithmetique_pointeur) {
				auto comme_type = [&](NoeudExpression *expr_ptr, Type *type)
				{
					auto comme = assem->cree_comme(expr_ptr->lexeme);
					comme->type = type;
					comme->expression = expr_ptr;
					comme->transformation = { TypeTransformation::POINTEUR_VERS_ENTIER, type };
					return comme;
				};

				auto type1 = expr_bin->expr1->type;
				auto type2 = expr_bin->expr2->type;

				// ptr - ptr => (ptr comme z64 - ptr comme z64) / taille_de(type_pointe)
				if (type1->est_pointeur() && type2->est_pointeur()) {
					auto const &type_z64 = typeuse[TypeBase::Z64];
					auto type_pointe = type2->comme_pointeur()->type_pointe;
					auto soustraction = assem->cree_op_binaire(expr_bin->lexeme, type_z64->operateur_sst, comme_type(expr_bin->expr1, type_z64), comme_type(expr_bin->expr2, type_z64));
					auto taille_de = assem->cree_lit_entier(expr_bin->lexeme, type_z64, type_pointe->taille_octet);
					auto div = assem->cree_op_binaire(expr_bin->lexeme, type_z64->operateur_div, soustraction, taille_de);
					expr_bin->substitution = div;
				}
				else {
					Type *type_entier = Type::nul();
					Type *type_pointeur = Type::nul();

					NoeudExpression *expr_entier = nullptr;
					NoeudExpression *expr_pointeur = nullptr;

					// ent + ptr => (ptr comme type_entier + ent * taille_de(type_pointe)) comme type_ptr
					if (est_type_entier(type1)) {
						type_entier = type1;
						type_pointeur = type2;
						expr_entier = expr_bin->expr1;
						expr_pointeur = expr_bin->expr2;
					}
					// ptr - ent => (ptr comme type_entier - ent * taille_de(type_pointe)) comme type_ptr
					// ptr + ent => (ptr comme type_entier + ent * taille_de(type_pointe)) comme type_ptr
					else if (est_type_entier(type2)) {
						type_entier = type2;
						type_pointeur = type1;
						expr_entier = expr_bin->expr2;
						expr_pointeur = expr_bin->expr1;
					}

					auto type_pointe = type_pointeur->comme_pointeur()->type_pointe;

					auto taille_de = assem->cree_lit_entier(expr_entier->lexeme, type_entier, type_pointe->taille_octet);
					auto mul = assem->cree_op_binaire(expr_entier->lexeme, type_entier->operateur_mul, expr_entier, taille_de);

					OperateurBinaire *op_arithm = nullptr;

					if (expr_bin->lexeme->genre == GenreLexeme::MOINS || expr_bin->lexeme->genre == GenreLexeme::MOINS_EGAL) {
						op_arithm = type_entier->operateur_sst;
					}
					else if (expr_bin->lexeme->genre == GenreLexeme::PLUS || expr_bin->lexeme->genre == GenreLexeme::PLUS_EGAL) {
						op_arithm = type_entier->operateur_ajt;
					}

					auto arithm = assem->cree_op_binaire(expr_bin->lexeme, op_arithm, comme_type(expr_pointeur, type_entier), mul);

					auto comme_pointeur = assem->cree_comme(expr_bin->lexeme);
					comme_pointeur->type = type_pointeur;
					comme_pointeur->expression = arithm;
					comme_pointeur->transformation = { TypeTransformation::ENTIER_VERS_POINTEUR, type_pointeur };

					expr_bin->substitution = comme_pointeur;
				}

				if (expr_bin->possede_drapeau(EST_ASSIGNATION_COMPOSEE)) {
					expr_bin->substitution = assem->cree_assignation(expr_bin->lexeme, expr_bin->expr1, expr_bin->substitution);
				}

				return;
			}

			if (expr_bin->possede_drapeau(EST_ASSIGNATION_COMPOSEE)) {
				noeud->substitution = assem->cree_assignation(expr_bin->lexeme, expr_bin->expr1, simplifie_operateur_binaire(expr_bin, true));
				return;
			}

			if (dls::outils::est_element(noeud->lexeme->genre, GenreLexeme::BARRE_BARRE, GenreLexeme::ESP_ESP)) {
				// À FAIRE : simplifie les accès à des énum_drapeaux dans les expressions || ou &&, il faudra également modifier la RI pour prendre en compte la substitution
				return;
			}

			noeud->substitution = simplifie_operateur_binaire(expr_bin, false);
			return;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr_un = noeud->comme_operateur_unaire();

			if (expr_un->type->est_type_de_donnees()) {
				expr_un->substitution = assem->cree_ref_type(expr_un->lexeme, expr_un->type);
				return;
			}

			simplifie(expr_un->expr);

			/* op peut être nul pour les opérateurs ! et * */
			if (expr_un->op && !expr_un->op->est_basique) {
				auto appel = assem->cree_appel(expr_un->lexeme, expr_un->op->decl, expr_un->op->type_resultat);
				appel->exprs.ajoute(expr_un->expr);
				expr_un->substitution = appel;
				return;
			}

			return;
		}
		case GenreNoeud::EXPRESSION_TYPE_DE:
		{
			/* change simplement le genre du noeud car le type de l'expression est le type de sa sous expression */
			noeud->genre = GenreNoeud::EXPRESSION_REFERENCE_TYPE;
			return;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			auto cuisine = noeud->comme_cuisine();
			auto expr = cuisine->expr->comme_appel();
			cuisine->substitution = assem->cree_ref_decl(expr->lexeme, expr->appelee->comme_entete_fonction());
			return;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			auto inst = noeud->comme_si_statique();

			if (inst->condition_est_vraie) {
				simplifie(inst->bloc_si_vrai);
				inst->substitution = inst->bloc_si_vrai;
				return;
			}

			simplifie(inst->bloc_si_faux);
			inst->substitution = inst->bloc_si_faux;
			return;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto expr_ref = noeud->comme_ref_decl();
			auto decl_ref = expr_ref->decl;

			if (decl_ref->drapeaux & EST_CONSTANTE) {
				auto decl_const = decl_ref->comme_decl_var();

				if (decl_ref->type->est_reel()) {
					expr_ref->substitution = assem->cree_lit_reel(expr_ref->lexeme, decl_ref->type, decl_const->valeur_expression.reel);
					return;
				}

				expr_ref->substitution = assem->cree_lit_entier(expr_ref->lexeme, decl_ref->type, static_cast<unsigned long>(decl_const->valeur_expression.entier));
				return;
			}

			if (dls::outils::est_element(decl_ref->genre, GenreNoeud::DECLARATION_ENUM, GenreNoeud::DECLARATION_STRUCTURE)) {
				expr_ref->substitution = assem->cree_ref_type(expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
				return;
			}

			return;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto ref_membre = noeud->comme_ref_membre();
			auto accede = ref_membre->accede;
			auto type_accede = accede->type;

			if (ref_membre->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
				/* Devraient être simplifié là où ils sont utilisés. */
				if (!ref_membre->possede_drapeau(DROITE_CONDITION)) {
					return;
				}

				// a.DRAPEAU => (a & DRAPEAU) != 0
				auto type_enum = ref_membre->type->comme_enum();
				auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

				auto valeur_lit_enum = assem->cree_lit_entier(noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
				auto op = type_enum->operateur_etb;
				auto et = assem->cree_op_binaire(noeud->lexeme, op, accede, valeur_lit_enum);

				auto zero = assem->cree_lit_entier(noeud->lexeme, type_enum, 0);
				op = type_enum->operateur_dif;
				auto dif = assem->cree_op_binaire(noeud->lexeme, op, et, zero);

				ref_membre->substitution = dif;
				return;
			}

			while (type_accede->est_pointeur() || type_accede->est_reference()) {
				type_accede = type_dereference_pour(type_accede);
			}

			if (type_accede->est_opaque()) {
				type_accede = type_accede->comme_opaque()->type_opacifie;
			}

			if (type_accede->est_tableau_fixe()) {
				auto taille = type_accede->comme_tableau_fixe()->taille;
				noeud->substitution = assem->cree_lit_entier(noeud->lexeme, noeud->type, static_cast<unsigned long>(taille));
				return;
			}

			if (type_accede->est_enum() || type_accede->est_erreur()) {
				auto type_enum = type_accede->comme_enum();
				auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
				noeud->substitution = assem->cree_lit_entier(noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
				return;
			}

			if (type_accede->est_type_de_donnees() && noeud->genre_valeur == GenreValeur::DROITE) {
				noeud->substitution = assem->cree_ref_type(noeud->lexeme, typeuse.type_type_de_donnees(noeud->type));
				return;
			}

			auto type_compose = static_cast<TypeCompose *>(type_accede);
			auto &membre = type_compose->membres[ref_membre->index_membre];

			if (membre.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
				simplifie(membre.expression_valeur_defaut);
				noeud->substitution = membre.expression_valeur_defaut;
				return;
			}

			/* pour les appels de fonctions */
			simplifie(ref_membre->accede);

			return;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr = noeud->comme_taille();
			auto type = expr->expr->type;
			noeud->substitution = assem->cree_lit_entier(noeud->lexeme, expr->type, type->taille_octet);
			return;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto inst = noeud->comme_comme();
			auto expr = inst->expression;

			if (expr->type == inst->type) {
				simplifie(expr);
				noeud->substitution = expr;
				return;
			}

			simplifie(inst->expression);
			return;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			simplifie_boucle_pour(noeud->comme_pour());
			return;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto boucle = noeud->comme_boucle();
			simplifie(boucle->bloc);
			return;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			/*

			  boucle {
				corps:
					...

				si condition {
					arrête (implicite)
				}
			  }

			 */
			auto boucle = noeud->comme_repete();
			simplifie(boucle->condition);
			simplifie(boucle->bloc);

			auto nouvelle_boucle = assem->cree_boucle(noeud->lexeme);

			auto condition = cree_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SAUFSI);
			condition->condition = boucle->condition;

			auto nouveau_bloc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
			nouveau_bloc->expressions->ajoute(boucle->bloc);
			nouveau_bloc->expressions->ajoute(condition);

			nouvelle_boucle->bloc = nouveau_bloc;
			boucle->substitution = nouvelle_boucle;
			return;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			/*

			  boucle {
				si condition {
					arrête (implicite)
				}

				corps:
					...

			  }

			 */
			auto boucle = noeud->comme_tantque();
			simplifie(boucle->condition);
			simplifie(boucle->bloc);

			auto nouvelle_boucle = assem->cree_boucle(noeud->lexeme);

			auto condition = cree_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SAUFSI);
			condition->condition = boucle->condition;

			auto nouveau_bloc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
			nouveau_bloc->expressions->ajoute(condition);
			nouveau_bloc->expressions->ajoute(boucle->bloc);

			nouvelle_boucle->bloc = nouveau_bloc;
			boucle->substitution = nouvelle_boucle;
			return;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto tableau = noeud->comme_construction_tableau();
			simplifie(tableau->expr);
			return;
		}
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto virgule = noeud->comme_virgule();

			POUR (virgule->expressions) {
				simplifie(it);
			}

			return;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			simplifie_retiens(noeud->comme_retiens());
			return;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto discr = noeud->comme_discr();
			simplifie_discr(discr);
			return;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto parenthese = noeud->comme_parenthese();
			simplifie(parenthese->expr);
			parenthese->substitution = parenthese->expr;
			return;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto tente = noeud->comme_tente();
			simplifie(tente->expr_appel);
			if (tente->bloc) {
				simplifie(tente->bloc);
			}
			return;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto tableau = noeud->comme_args_variadiques();

			POUR (tableau->exprs) {
				simplifie(it);
			}

			return;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto retour = noeud->comme_retour();
			simplifie_retour(retour);
			return;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto si = static_cast<NoeudSi *>(noeud);
			simplifie(si->condition);
			simplifie(si->bloc_si_vrai);
			simplifie(si->bloc_si_faux);

			if (si->possede_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
				/*

				  x := si y { z } sinon { w }

				  {
					decl := XXX;
					si y { decl = z; } sinon { decl = w; }
					decl; // nous avons une référence simple car la RI empilera sa valeur qui pourra être dépilée et utilisée pour l'assignation
				  }

				 */

				auto bloc = assem->cree_bloc_seul(si->lexeme, si->bloc_parent);

				auto decl_temp = assem->cree_declaration(si->lexeme, si->type, nullptr, nullptr);
				auto ref_temp = assem->cree_ref_decl(si->lexeme, decl_temp);

				auto nouveau_si = assem->cree_si(si->lexeme, si->genre);
				nouveau_si->condition = si->condition;
				nouveau_si->bloc_si_vrai = si->bloc_si_vrai;
				nouveau_si->bloc_si_faux = si->bloc_si_faux;

				corrige_bloc_pour_assignation(nouveau_si->bloc_si_vrai, ref_temp);
				corrige_bloc_pour_assignation(nouveau_si->bloc_si_faux, ref_temp);

				bloc->membres->ajoute(decl_temp);
				bloc->expressions->ajoute(decl_temp);
				bloc->expressions->ajoute(nouveau_si);
				bloc->expressions->ajoute(ref_temp);

				si->substitution = bloc;
			}

			return;
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			simplifie_comparaison_chainee(noeud->comme_comparaison_chainee());
			return;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto appel = noeud->comme_construction_struct();

			POUR (appel->exprs) {
				simplifie(it);
			}

			return;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto appel = noeud->comme_appel();

			if (appel->aide_generation_code == CONSTRUIT_OPAQUE) {
				simplifie(appel->exprs[0]);

				auto comme = assem->cree_comme(appel->lexeme);
				comme->type = appel->type;
				comme->expression = appel->exprs[0];
				comme->transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, appel->type };
				comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

				appel->substitution = comme;
				return;
			}

			if (appel->aide_generation_code == MONOMORPHE_TYPE_OPAQUE) {
				appel->substitution = assem->cree_ref_type(appel->lexeme, appel->type);
			}

			if (appel->noeud_fonction_appelee) {
				appel->appelee->substitution = assem->cree_ref_decl(appel->lexeme, const_cast<NoeudDeclarationEnteteFonction *>(appel->noeud_fonction_appelee->comme_entete_fonction()));
			}
			else {
				simplifie(appel->appelee);
			}

			POUR (appel->exprs) {
				simplifie(it);
			}

			return;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto assignation = noeud->comme_assignation();

			simplifie(assignation->variable);

			POUR (assignation->donnees_exprs) {
				auto expression_fut_simplifiee = false;

				for (auto var : it.variables) {
					if (var->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
						auto ref_membre = var->comme_ref_membre();
						auto ref_var = ref_membre->accede;

						// À FAIRE : référence
						auto nouvelle_ref = assem->cree_ref_decl(ref_var->lexeme, ref_var->comme_ref_decl()->decl);
						var->substitution = nouvelle_ref;

						auto type_enum = ref_membre->type->comme_enum();
						auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

						if (it.expression->comme_litterale()->valeur_bool) {
							// a.DRAPEAU = vrai -> a = a | DRAPEAU
							auto valeur_lit_enum = assem->cree_lit_entier(noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
							auto op = type_enum->operateur_oub;
							auto ou = assem->cree_op_binaire(noeud->lexeme, op, nouvelle_ref, valeur_lit_enum);
							it.expression->substitution = ou;
						}
						else {
							// a.DRAPEAU = faux -> a = a & ~DRAPEAU
							auto valeur_lit_enum = assem->cree_lit_entier(noeud->lexeme, type_enum, ~static_cast<unsigned>(valeur_enum));
							auto op = type_enum->operateur_etb;
							auto et = assem->cree_op_binaire(noeud->lexeme, op, nouvelle_ref, valeur_lit_enum);
							it.expression->substitution = et;
						}

						expression_fut_simplifiee = true;
					}
				}

				if (!expression_fut_simplifiee) {
					simplifie(it.expression);
				}
			}

			return;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto declaration = noeud->comme_decl_var();

			POUR (declaration->donnees_decl) {
				simplifie(it.expression);
			}

			return;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto pousse_contexte = noeud->comme_pousse_contexte();
			simplifie(pousse_contexte->bloc);
			return;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto indexage = noeud->comme_indexage();
			simplifie(indexage->expr1);
			simplifie(indexage->expr2);

			if (indexage->op) {
				indexage->substitution = simplifie_operateur_binaire(indexage, true);
			}

			return;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto structure = noeud->comme_structure();
			auto type = static_cast<TypeCompose *>(structure->type);

			POUR (type->membres) {
				simplifie(it.expression_valeur_defaut);
			}

			return;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_EMPL:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::INSTRUCTION_CHARGE:
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			return;
		}
	}

	return;
}

static OperateurBinaire const *operateur_pour_lexeme(GenreLexeme lexeme_op, Type *type, bool plage)
{
	/* NOTE : les opérateurs sont l'inverse de ce qu'indique les lexèmes car la condition est inversée. */
	if (lexeme_op == GenreLexeme::INFERIEUR) {
		if (plage) {
			return type->operateur_sup;
		}

		return type->operateur_seg;
	}

	if (lexeme_op == GenreLexeme::SUPERIEUR) {
		if (plage) {
			return type->operateur_inf;
		}

		return type->operateur_ieg;
	}

	return nullptr;
}

void Simplificatrice::simplifie_boucle_pour(NoeudPour *inst)
{
	simplifie(inst->bloc);
	simplifie(inst->bloc_sansarret);
	simplifie(inst->bloc_sinon);

	auto feuilles = inst->variable->comme_virgule();

	auto var = feuilles->expressions[0];
	auto idx = NoeudExpression::nul();
	auto expression_iteree = inst->expression;
	auto bloc = inst->bloc;
	auto bloc_sans_arret = inst->bloc_sansarret;
	auto bloc_sinon = inst->bloc_sinon;

	auto ident_index_it = ID::index_it;
	auto type_index_it = typeuse[TypeBase::Z64];
	if (feuilles->expressions.taille == 2) {
		idx = feuilles->expressions[1];
		ident_index_it = idx->ident;
		type_index_it = idx->type;
	}

	auto boucle = assem->cree_boucle(inst->lexeme);
	boucle->ident = var->ident;
	boucle->bloc = assem->cree_bloc_seul(inst->lexeme, boucle->bloc_parent);
	boucle->bloc_sansarret = bloc_sans_arret;
	boucle->bloc_sinon = bloc_sinon;

	auto it = var->comme_decl_var();

	auto zero = assem->cree_lit_entier(var->lexeme, type_index_it, 0);
	auto index_it = idx ? idx->comme_decl_var() : assem->cree_declaration(var->lexeme, type_index_it, ident_index_it, zero);

	auto ref_it = it->valeur->comme_ref_decl();
	auto ref_index = index_it->valeur->comme_ref_decl();

	auto bloc_pre = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
	bloc_pre->membres->ajoute(it);
	bloc_pre->membres->ajoute(index_it);

	bloc_pre->expressions->ajoute(it);
	bloc_pre->expressions->ajoute(index_it);

	auto bloc_inc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);

	auto condition = cree_condition_boucle(inst, GenreNoeud::INSTRUCTION_SI);

	boucle->bloc_pre = bloc_pre;
	boucle->bloc_inc = bloc_inc;

	auto const inverse_boucle = inst->lexeme_op == GenreLexeme::SUPERIEUR;

	auto type_itere = expression_iteree->type->est_opaque() ? expression_iteree->type->comme_opaque()->type_opacifie : expression_iteree->type;
	expression_iteree->type = type_itere;

	/* boucle */

	switch (inst->aide_generation_code) {
		case GENERE_BOUCLE_PLAGE:
		case GENERE_BOUCLE_PLAGE_INDEX:
		{
			/*

			  pour 0 ... 10 {
				corps
			  }

			  it := 0
			  index_it := 0

			  boucle {
				si it > 10 {
					arrête
				}

				corps:
				  ...

				incrémente:
					it = it + 1
					index_it = index_it + 1
			  }

			 */

			/* condition */
			auto expr_plage = expression_iteree->comme_plage();
			simplifie(expr_plage->expr1);
			simplifie(expr_plage->expr2);

			auto expr_debut = inverse_boucle ? expr_plage->expr2 : expr_plage->expr1;
			auto expr_fin   = inverse_boucle ? expr_plage->expr1 : expr_plage->expr2;

			auto init_it = assem->cree_assignation(ref_it->lexeme, ref_it, expr_debut);
			bloc_pre->expressions->ajoute(init_it);

			auto op_comp = operateur_pour_lexeme(inst->lexeme_op, ref_it->type, true);
			condition->condition = assem->cree_op_binaire(inst->lexeme, op_comp, ref_it, expr_fin);
			boucle->bloc->expressions->ajoute(condition);

			/* corps */
			boucle->bloc->expressions->ajoute(bloc);

			/* suivant */
			if (inverse_boucle) {
				auto inc_it = assem->cree_decrementation(ref_it->lexeme, ref_it);
				bloc_inc->expressions->ajoute(inc_it);
			}
			else {
				auto inc_it = assem->cree_incrementation(ref_it->lexeme, ref_it);
				bloc_inc->expressions->ajoute(inc_it);
			}

			auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
			bloc_inc->expressions->ajoute(inc_it);

			break;
		}
		case GENERE_BOUCLE_TABLEAU:
		case GENERE_BOUCLE_TABLEAU_INDEX:
		{
			/*

			  pour chn {
				corps
			  }

			  index_it := 0
			  boucle {
				si index_it >= chn.taille {
					arrête
				}

				it := chn.pointeur[index_it]

				corps:
				  ...

				incrémente:
					index_it = index_it + 1
			  }

			 */

			/* condition */
			auto expr_taille = NoeudExpression::nul();

			if (type_itere->est_tableau_fixe()) {
				auto taille_tableau = type_itere->comme_tableau_fixe()->taille;
				expr_taille = assem->cree_lit_entier(inst->lexeme, typeuse[TypeBase::Z64], static_cast<unsigned long>(taille_tableau));
			}
			else {
				expr_taille = assem->cree_acces_membre(inst->lexeme, expression_iteree, typeuse[TypeBase::Z64], 1);
			}

			auto type_z64 = typeuse[TypeBase::Z64];
			condition->condition = assem->cree_op_binaire(inst->lexeme, type_z64->operateur_seg, ref_index, expr_taille);

			auto expr_pointeur = NoeudExpression::nul();		

			auto type_compose = type_itere->comme_compose();

			if (type_itere->est_tableau_fixe()) {
				auto indexage = assem->cree_indexage(inst->lexeme, expression_iteree, zero, true);

				static const Lexeme lexeme_adresse = { ",", {}, GenreLexeme::FOIS_UNAIRE, 0, 0, 0 };
				auto prise_adresse = assem->cree_op_unaire(&lexeme_adresse);
				prise_adresse->expr = indexage;
				prise_adresse->type = typeuse.type_pointeur_pour(indexage->type);

				expr_pointeur = prise_adresse;
			}
			else {
				expr_pointeur = assem->cree_acces_membre(inst->lexeme, expression_iteree, type_compose->membres[0].type, 0);
			}

			NoeudExpression *expr_index = ref_index;

			if (inverse_boucle) {
				expr_index = assem->cree_op_binaire(inst->lexeme, ref_index->type->operateur_sst, expr_taille, ref_index);
				expr_index = assem->cree_op_binaire(inst->lexeme, ref_index->type->operateur_sst, expr_index, assem->cree_lit_entier(ref_index->lexeme, ref_index->type, 1));
			}

			auto indexage = assem->cree_indexage(inst->lexeme, expr_pointeur, expr_index, true);
			NoeudExpression *expression_assignee = indexage;

			if (inst->prend_reference || inst->prend_pointeur) {
				auto noeud_comme = assem->cree_comme(it->lexeme);
				noeud_comme->type = it->type;
				noeud_comme->expression = indexage;
				noeud_comme->transformation = TypeTransformation::PREND_REFERENCE;
				noeud_comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

				expression_assignee = noeud_comme;
			}

			auto assign_it = assem->cree_assignation(inst->lexeme, ref_it, expression_assignee);

			boucle->bloc->expressions->ajoute(condition);
			boucle->bloc->expressions->ajoute(assign_it);

			/* corps */
			boucle->bloc->expressions->ajoute(bloc);

			/* incrémente */
			auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
			bloc_inc->expressions->ajoute(inc_it);
			break;
		}
		case GENERE_BOUCLE_COROUTINE:
		case GENERE_BOUCLE_COROUTINE_INDEX:
		{
			/* À FAIRE(ri) : coroutine */
#if 0
			auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
			auto decl_fonc = static_cast<NoeudDeclarationCorpsFonction const *>(expr_appel->noeud_fonction_appelee);
			auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
			auto nom_type_coro = "__etat_coro" + decl_fonc->nom_broye;

			constructrice << nom_type_coro << " " << nom_etat << " = {\n";
			constructrice << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".contexte = contexte,\n";
			constructrice << ".__termine_coro = 0\n";
			constructrice << "};\n";

			/* intialise les arguments de la fonction. */
			POUR (expr_appel->params) {
				genere_code_C(it, constructrice, compilatrice, false);
			}

			auto iter_enf = expr_appel->params.begin();

			POUR (decl_fonc->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				constructrice << nom_etat << '.' << nom_broye << " = ";
				constructrice << (*iter_enf)->chaine_calculee();
				constructrice << ";\n";
				++iter_enf;
			}

			constructrice << "pthread_t fil_coro;\n";
			constructrice << "pthread_create(&fil_coro, NULL, " << decl_fonc->nom_broye << ", &" << nom_etat << ");\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			/* À FAIRE : utilisation du type */
			auto nombre_vars_ret = decl_fonc->type_fonc->types_sorties.taille;

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant1, feuilles);

			auto idx = static_cast<NoeudExpression *>(nullptr);
			auto nom_idx = dls::chaine{};

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				idx = feuilles.back();
				nom_idx = "__idx" + dls::vers_chaine(b);
				constructrice << "int " << nom_idx << " = 0;";
			}

			constructrice << "while (" << nom_etat << ".__termine_coro == 0) {\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

			for (auto i = 0l; i < nombre_vars_ret; ++i) {
				auto f = feuilles[i];
				auto nom_var_broye = broye_chaine(f);
				constructrice.declare_variable(type, nom_var_broye, "");
				constructrice << nom_var_broye << " = "
							   << nom_etat << '.' << decl_fonc->noms_retours[i]
							   << ";\n";
			}

			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			if (idx) {
				constructrice << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
				constructrice << nom_idx << " += 1;";
			}
#endif
			break;
		}
	}

	inst->substitution = boucle;
}

static void rassemble_operations_chainees(
		NoeudExpression *racine,
		kuri::tableau<NoeudExpressionBinaire> &comparaisons)
{
	auto expr_bin = static_cast<NoeudExpressionBinaire *>(racine);

	if (est_operateur_comp(expr_bin->expr1->lexeme->genre)) {
		rassemble_operations_chainees(expr_bin->expr1, comparaisons);

		auto expr_operande = static_cast<NoeudExpressionBinaire *>(expr_bin->expr1);

		auto comparaison = NoeudExpressionBinaire{};
		comparaison.lexeme = expr_bin->lexeme;
		comparaison.expr1 = expr_operande->expr2;
		comparaison.expr2 = expr_bin->expr2;
		comparaison.op = expr_bin->op;
		comparaison.permute_operandes = expr_bin->permute_operandes;

		comparaisons.ajoute(comparaison);
	}
	else {
		auto comparaison = NoeudExpressionBinaire{};
		comparaison.lexeme = expr_bin->lexeme;
		comparaison.expr1 = expr_bin->expr1;
		comparaison.expr2 = expr_bin->expr2;
		comparaison.op = expr_bin->op;
		comparaison.permute_operandes = expr_bin->permute_operandes;

		comparaisons.ajoute(comparaison);
	}
}

NoeudExpression *Simplificatrice::cree_expression_pour_op_chainee(
		kuri::tableau<NoeudExpressionBinaire> &comparaisons,
		Lexeme const *lexeme_op_logique)
{
	dls::pile<NoeudExpression *> exprs;

	for (auto i = comparaisons.taille - 1; i >= 0; --i) {
		auto &it = comparaisons[i];
		simplifie(it.expr1);
		simplifie(it.expr2);
		exprs.empile(simplifie_operateur_binaire(&it, true));
	}

	if (exprs.taille() == 1) {
		return exprs.depile();
	}

	auto resultat = NoeudExpression::nul();

	while (true) {
		auto a = exprs.depile();
		auto b = exprs.depile();

		auto et = assem->cree_op_binaire(lexeme_op_logique);
		et->expr1 = a;
		et->expr2 = b;

		if (exprs.est_vide()) {
			resultat = et;
			break;
		}

		exprs.empile(et);
	}

	return resultat;
}

void Simplificatrice::corrige_bloc_pour_assignation(NoeudExpression *expr, NoeudExpression *ref_temp)
{
	if (expr->est_bloc()) {
		auto bloc = expr->comme_bloc();

		auto di = bloc->expressions->derniere();
		di = assem->cree_assignation(di->lexeme, ref_temp, di);
		bloc->expressions->supprime_dernier();
		bloc->expressions->ajoute(di);
	}
	else if (expr->est_si()) {
		auto si = expr->comme_si();
		corrige_bloc_pour_assignation(si->bloc_si_vrai, ref_temp);
		corrige_bloc_pour_assignation(si->bloc_si_faux, ref_temp);
	}
	else if (expr->est_saufsi()) {
		auto si = expr->comme_saufsi();
		corrige_bloc_pour_assignation(si->bloc_si_vrai, ref_temp);
		corrige_bloc_pour_assignation(si->bloc_si_faux, ref_temp);
	}
	else {
		assert_rappel(false, [&](){
			erreur::imprime_site(*espace, expr);
			std::cerr << "Expression invalide pour la simplification de l'assignation implicite d'un bloc si !\n";
		});
	}
}

void Simplificatrice::simplifie_comparaison_chainee(NoeudExpressionBinaire *comp)
{
	auto comparaisons = kuri::tableau<NoeudExpressionBinaire>();
	rassemble_operations_chainees(comp, comparaisons);

	/*
	  a <= b <= c

	  a <= b && b <= c && c <= d

	  &&
		&&
		  a <= b
		  b <= c
		c <= d
	 */

	static const Lexeme lexeme_et = { ",", {}, GenreLexeme::ESP_ESP, 0, 0, 0 };
	comp->substitution = cree_expression_pour_op_chainee(comparaisons, &lexeme_et);
}

/* Les retours sont simplifiés sous forme d'assignations des valeurs de retours,
 * et d'un chargement pour les retours simples. */
void Simplificatrice::simplifie_retour(NoeudRetour *inst)
{
	// crée une assignation pour chaque sortie
	auto type_fonction = fonction_courante->type->comme_fonction();
	auto type_sortie = type_fonction->type_sortie;

	if (type_sortie->est_rien()) {
		return;
	}

	POUR (inst->donnees_exprs) {
		simplifie(it.expression);

		/* Les variables sont les déclarations des paramètres, donc crée des références. */
		for (auto &var : it.variables) {
			var = assem->cree_ref_decl(var->lexeme, var->comme_decl_var());
		}
	}

	auto assignation = assem->cree_assignation(inst->lexeme);
	assignation->expression = inst->expr;
	assignation->donnees_exprs = std::move(inst->donnees_exprs);

	auto retour = assem->cree_retour(inst->lexeme);

	if (type_sortie->est_rien()) {
		retour->expr = nullptr;
	}
	else {
		retour->expr = assem->cree_ref_decl(fonction_courante->param_sortie->lexeme, fonction_courante->param_sortie);
	}

	auto bloc = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);
	bloc->expressions->ajoute(assignation);
	bloc->expressions->ajoute(retour);
	retour->bloc_parent = bloc;

	inst->substitution = bloc;
	return;
}

NoeudExpression *Simplificatrice::simplifie_operateur_binaire(NoeudExpressionBinaire *expr_bin, bool pour_operande)
{
	if (expr_bin->op->est_basique) {
		if (!pour_operande) {
			return nullptr;
		}

		/* Crée une nouvelle expression binaire afin d'éviter les dépassements de piles car
		 * sinon la substitution serait toujours réévaluée lors de l'évaluation de l'expression
		 * d'assignation. */
		return assem->cree_op_binaire(expr_bin->lexeme, expr_bin->op, expr_bin->expr1, expr_bin->expr2);
	}

	auto appel = assem->cree_appel(expr_bin->lexeme, expr_bin->op->decl, expr_bin->op->type_resultat);

	if (expr_bin->permute_operandes) {
		appel->exprs.ajoute(expr_bin->expr2);
		appel->exprs.ajoute(expr_bin->expr1);
	}
	else {
		appel->exprs.ajoute(expr_bin->expr1);
		appel->exprs.ajoute(expr_bin->expr2);
	}

	return appel;
}

void Simplificatrice::simplifie_coroutine(NoeudDeclarationEnteteFonction *corout)
{
#if 0
	compilatrice.commence_fonction(decl);

	/* Crée fonction */
	auto nom_fonction = decl->nom_broye;
	auto nom_type_coro = "__etat_coro" + nom_fonction;

	/* Déclare la structure d'état de la coroutine. */
	constructrice << "typedef struct " << nom_type_coro << " {\n";
	constructrice << "pthread_mutex_t mutex_boucle;\n";
	constructrice << "pthread_cond_t cond_boucle;\n";
	constructrice << "pthread_mutex_t mutex_coro;\n";
	constructrice << "pthread_cond_t cond_coro;\n";
	constructrice << "bool __termine_coro;\n";
	constructrice << "ContexteProgramme contexte;\n";

	auto idx_ret = 0l;
	POUR (decl->type_fonc->types_sorties) {
		auto &nom_ret = decl->noms_retours[idx_ret++];
		constructrice.declare_variable(it, nom_ret, "");
	}

	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);
		constructrice.declare_variable(it->type, nom_broye, "");
	}

	constructrice << " } " << nom_type_coro << ";\n";

	/* Déclare la fonction. */
	constructrice << "static void *" << nom_fonction << "(\nvoid *data)\n";
	constructrice << "{\n";
	constructrice << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
	constructrice << "ContexteProgramme contexte = __etat->contexte;\n";

	/* déclare les paramètres. */
	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);
		constructrice.declare_variable(it->type, nom_broye, "__etat->" + nom_broye);
	}

	/* Crée code pour le bloc. */
	genere_code_C(decl->bloc, constructrice, compilatrice, false);

	if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
		genere_code_extra_pre_retour(decl->bloc, compilatrice, constructrice);
	}

	constructrice << "}\n";

	compilatrice.termine_fonction();
	noeud->drapeaux |= RI_FUT_GENEREE;
#else
	static_cast<void>(corout);
#endif
	return;
}

void Simplificatrice::simplifie_retiens(NoeudRetour *retiens)
{
#if 0
	auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
	auto df = compilatrice.donnees_fonction;
	auto enfant = inst->expr;

	constructrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

	auto feuilles = dls::tablet<NoeudExpression *, 10>{};
	rassemble_feuilles(enfant, feuilles);

	for (auto i = 0l; i < feuilles.taille(); ++i) {
		auto f = feuilles[i];

		genere_code_C(f, constructrice, compilatrice, true);

		constructrice << "__etat->" << df->noms_retours[i] << " = ";
		constructrice << f->chaine_calculee();
		constructrice << ";\n";
	}

	constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";
#else
	static_cast<void>(retiens);
#endif
	return;
}

static auto trouve_index_membre(TypeCompose *type_compose, IdentifiantCode *nom_membre)
{
	auto idx_membre = 0u;

	POUR (type_compose->membres) {
		if (it.nom == nom_membre) {
			break;
		}

		idx_membre += 1;
	}

	return idx_membre;
}

static auto trouve_index_membre(TypeCompose *type_compose, Type *type_membre)
{
	auto idx_membre = 0u;

	POUR (type_compose->membres) {
		if (it.type == type_membre) {
			break;
		}

		idx_membre += 1;
	}

	return idx_membre;
}

static int valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident)
{
	auto decl_enum = type_enum->decl;
	auto index_membre = 0;

	POUR (*decl_enum->bloc->membres.verrou_lecture()) {
		if (it->ident == ident) {
			break;
		}

		index_membre += 1;
	}

	return type_enum->membres[index_membre].valeur;
}

enum {
	DISCR_UNION,
	DISCR_UNION_ANONYME,
	DISCR_DEFAUT,
	DISCR_ENUM,
};

template <int N>
void Simplificatrice::simplifie_discr_impl(NoeudDiscr *discr)
{
	/*

	  discr x {
		a { ... }
		b, c { ... }
		sinon { ... }
	  }

	  si x == a {
		...
	  }
	  sinon si x == b || x == c {
		...
	  }
	  sinon {
		...
	  }

	 */

	static const Lexeme lexeme_ou = { ",", {}, GenreLexeme::BARRE_BARRE, 0, 0, 0 };

	auto si = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
	auto si_courant = si;

	discr->substitution = si;

	auto expression = NoeudExpression::nul();
	if (N == DISCR_UNION || N == DISCR_UNION_ANONYME) {
		/* nous utilisons directement un accès de membre... il faudra proprement gérer les unions */
		expression = assem->cree_acces_membre(discr->expr->lexeme, discr->expr, typeuse[TypeBase::Z32], 1);
	}
	else {
		expression = discr->expr;
	}

	for (auto i = 0; i < discr->paires_discr.taille; ++i) {
		auto &it = discr->paires_discr[i];
		auto virgule = it.first->comme_virgule();

		// crée les comparaisons
		kuri::tableau<NoeudExpressionBinaire> comparaisons;

		for (auto expr : virgule->expressions) {
			auto comparaison = NoeudExpressionBinaire();
			comparaison.lexeme = discr->lexeme;
			comparaison.op = discr->op;
			comparaison.expr1 = expression;

			if (N == DISCR_ENUM) {
				auto valeur = valeur_enum(expression->type->comme_enum(), expr->ident);
				auto constante = assem->cree_lit_entier(expr->lexeme, expression->type, static_cast<unsigned long>(valeur));
				comparaison.expr2 = constante;
			}
			else if (N == DISCR_UNION) {
				auto const type_union = discr->expr->type->comme_union();
				auto index = trouve_index_membre(type_union, expr->ident);
				auto constante = assem->cree_lit_entier(expr->lexeme, expression->type, static_cast<unsigned long>(index + 1));
				comparaison.expr2 = constante;
			}
			else if (N == DISCR_UNION_ANONYME) {
				auto const type_union = discr->expr->type->comme_union();
				auto index = trouve_index_membre(type_union, expr->type);
				auto constante = assem->cree_lit_entier(expr->lexeme, expression->type, static_cast<unsigned long>(index + 1));
				comparaison.expr2 = constante;
			}
			else {
				/* cette expression est simplifiée via cree_expression_pour_op_chainee */
				comparaison.expr2 = expr;
			}

			comparaisons.ajoute(comparaison);
		}

		si_courant->condition = cree_expression_pour_op_chainee(comparaisons, &lexeme_ou);

		// À FAIRE(union) : création d'une variable si nous avons une union
		simplifie(it.second);
		si_courant->bloc_si_vrai = it.second;

		if (i != (discr->paires_discr.taille - 1)) {
			si = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
			si_courant->bloc_si_faux = si;
			si_courant = si;
		}
	}

	simplifie(discr->bloc_sinon);
	si_courant->bloc_si_faux = discr->bloc_sinon;

#if 0
	/* génération du code pour l'expression contre laquelle nous testons */
	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
		valeur_f = valeur_enum(expression->type->comme_enum(), f->ident);
	}
	else if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
		auto type_union = noeud->expr->type->comme_union();
		if (type_union->est_anonyme) {
			unsigned idx_membre = 0;

			POUR (type_union->membres) {
				if (it.type == f->type) {
					break;
				}

				idx_membre += 1;
			}

			valeur_f = cree_z32(idx_membre + 1);

			/* ajout du membre au bloc */
			auto valeur = cree_acces_membre(noeud, ptr_structure, 0);
			table_locales[f->ident] = valeur;
		}
		else {
			auto idx_membre = trouve_index_membre(decl_struct, f->ident);
			valeur_f = cree_z32(idx_membre + 1);

			/* ajout du membre au bloc */
			auto valeur = cree_acces_membre(noeud, ptr_structure, 0);
			table_locales[f->ident] = valeur;
		}
	}
	else {
		genere_ri_pour_expression_droite(f, nullptr);
		valeur_f = depile_valeur();
	}

#endif
}


void Simplificatrice::simplifie_discr(NoeudDiscr *discr)
{
	if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
		auto const type_union = discr->expr->type->comme_union();

		if (type_union->est_anonyme) {
			simplifie_discr_impl<DISCR_UNION_ANONYME>(discr);
		}
		else {
			simplifie_discr_impl<DISCR_UNION>(discr);
		}
	}
	else if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
		simplifie_discr_impl<DISCR_ENUM>(discr);
	}
	else {
		simplifie_discr_impl<DISCR_DEFAUT>(discr);
	}
}

NoeudSi *Simplificatrice::cree_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud)
{
	static const Lexeme lexeme_arrete = { ",", {}, GenreLexeme::ARRETE, 0, 0, 0 };

	/* condition d'arrêt de la boucle */
	auto condition = assem->cree_si(inst->lexeme, genre_noeud);
	auto bloc_si_vrai = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);

	auto arrete = assem->cree_arrete(&lexeme_arrete);
	arrete->drapeaux |= EST_IMPLICITE;

	bloc_si_vrai->expressions->ajoute(arrete);
	condition->bloc_si_vrai = bloc_si_vrai;

	return condition;
}

NoeudDeclarationVariable *NoeudDeclarationEnteteFonction::parametre_entree(long i) const
{
	auto param = params[i];

	if (param->est_empl()) {
		return param->comme_empl()->expr->comme_decl_var();
	}

	return param->comme_decl_var();
}

dls::chaine const &NoeudDeclarationEnteteFonction::nom_broye(EspaceDeTravail *espace)
{
	if (nom_broye_ != "") {
		return nom_broye_;
	}

	if (ident != ID::principale && !possede_drapeau(EST_EXTERNE | FORCE_SANSBROYAGE)) {
		auto fichier = espace->fichier(lexeme->fichier);

		if (est_metaprogramme) {
			nom_broye_ = "metaprogramme" + dls::vers_chaine(this);
		}
		else {
			nom_broye_ = broye_nom_fonction(this, fichier->module->nom()->nom);
		}
	}
	else {
		nom_broye_ = lexeme->chaine;
	}

	return nom_broye_;
}

void simplifie_arbre(EspaceDeTravail *espace, AssembleuseArbre *assem, Typeuse &typeuse, NoeudExpression *arbre)
{
	auto simplificatrice = Simplificatrice(espace, assem, typeuse);
	simplificatrice.simplifie(arbre);
}
