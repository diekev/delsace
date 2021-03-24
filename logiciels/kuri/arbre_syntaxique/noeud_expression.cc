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

#include "noeud_expression.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "compilation/broyage.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/typage.hh"

#include "parsage/identifiant.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "assembleuse.hh"

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
		case GenreNoeud::DECLARATION_MODULE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto bloc = static_cast<NoeudBloc *>(racine);

			imprime_tab(os, tab);
			os << "bloc : " << bloc->expressions->taille() << " expression(s)\n";

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
			os << "decl var : ";
			if (expr->ident) {
				os << expr->ident->nom;
			}
			else {
				os << expr->lexeme->chaine;
			}
			os << "\n";

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
			auto expr = racine->comme_assignation_variable();

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

			imprime_arbre(expr->operande_gauche, os, tab + 1, substitution);
			imprime_arbre(expr->operande_droite, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre *>(racine);

			imprime_tab(os, tab);
			os << "expr membre : " << expr->lexeme->chaine << '\n';

			imprime_arbre(expr->accedee, os, tab + 1, substitution);
			imprime_arbre(expr->membre, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(racine);

			imprime_tab(os, tab);
			os << "expr appel : " << expr->lexeme->chaine << " (ident: " << racine->ident << ")" << '\n';

			imprime_arbre(expr->appelee, os, tab + 1, substitution);

			POUR (expr->parametres_resolus) {
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

			imprime_arbre(expr->operande, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_CHARGE:
		{
			auto inst = racine->comme_charge();
			imprime_tab(os, tab);
			os << "charge :\n";
			imprime_arbre(inst->operande, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_IMPORTE:
		{
			auto inst = racine->comme_importe();
			imprime_tab(os, tab);
			os << "importe :\n";
			imprime_arbre(inst->operande, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retourne();
			imprime_tab(os, tab);
			os << "retour : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expression, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			imprime_tab(os, tab);
			os << "retiens : " << inst->lexeme->chaine << '\n';
			imprime_arbre(inst->expression, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			auto cuisine = racine->comme_cuisine();
			imprime_tab(os, tab);
			os << "cuisine :\n";
			imprime_arbre(cuisine->operande, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTE:
		{
			auto expr = static_cast<NoeudDirectiveExecute *>(racine);

			imprime_tab(os, tab);
			os << "dir exécution :\n";

			imprime_arbre(expr->expression, os, tab + 1, substitution);
			break;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			imprime_tab(os, tab);
			os << "expr init_de\n";

			auto init_de = racine->comme_init_de();
			imprime_arbre(init_de->operande, os, tab + 1, substitution);

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
			os << "expr args variadiques : " << expr->expressions.taille() << " expression(s)\n";

			POUR (expr->expressions) {
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

			imprime_arbre(expr->expression_discriminee, os, tab + 1, substitution);

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

			imprime_arbre(expr->expression, os, tab + 1, substitution);
			imprime_arbre(expr->bloc, os, tab + 1, substitution);

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto inst = static_cast<NoeudInstructionTente *>(racine);

			os << "tente :\n";

			imprime_arbre(inst->expression_appelee, os, tab + 1, substitution);
			imprime_arbre(inst->expression_piegee, os, tab + 1, substitution);
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

	NoeudExpression *nracine = nullptr;
#define CREE_NOEUD_POUR_COPIE(genre_noeud) \
	nracine = assem->cree_noeud<genre_noeud>(racine->lexeme); \
	nracine->genre = racine->genre; \
	nracine->ident = racine->ident; \
	nracine->type = racine->type; \
	nracine->bloc_parent = bloc_parent; \
	nracine->drapeaux = racine->drapeaux; \
	nracine->drapeaux &= ~DECLARATION_FUT_VALIDEE

	switch (racine->genre) {
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_COMPOSEE);
			auto bloc = static_cast<NoeudBloc const *>(racine);
			auto nbloc = static_cast<NoeudBloc *>(nracine);
			nbloc->membres->reserve(bloc->membres->taille());
			nbloc->expressions->reserve(bloc->expressions->taille());
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DECLARATION_ENTETE_FONCTION);
			auto expr  = racine->comme_entete_fonction();
			auto nexpr = nracine->comme_entete_fonction();

			nexpr->params.reserve(expr->params.taille());
			nexpr->params_sorties.reserve(expr->params_sorties.taille());
			nexpr->arbre_aplatis.reserve(expr->arbre_aplatis.taille());
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

			if (expr->params_sorties.taille() > 1) {
				nexpr->param_sortie = copie_noeud(assem, expr->param_sortie, bloc_parent)->comme_declaration_variable();
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

				nexpr_corps->arbre_aplatis.reserve(expr_corps->arbre_aplatis.taille());
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DECLARATION_ENUM);
			auto decl = static_cast<NoeudEnum const *>(racine);
			auto ndecl = static_cast<NoeudEnum *>(nracine);

			ndecl->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, decl->bloc, bloc_parent));
			ndecl->expression_type = copie_noeud(assem, decl->expression_type, bloc_parent);
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DECLARATION_STRUCTURE);
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DECLARATION_VARIABLE);
			auto expr = static_cast<NoeudDeclarationVariable const *>(racine);
			auto nexpr = static_cast<NoeudDeclarationVariable *>(nracine);

			nexpr->valeur = copie_noeud(assem, expr->valeur, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->expression_type = copie_noeud(assem, expr->expression_type, bloc_parent);

			/* n'oublions pas de mettre en place les déclarations */
			if (nexpr->valeur->est_reference_declaration()) {
				nexpr->valeur->comme_reference_declaration()->declaration_referee = nexpr;
			}
			else if (nexpr->valeur->est_virgule()) {
				auto virgule = expr->valeur->comme_virgule();
				auto nvirgule = nexpr->valeur->comme_virgule();

				auto index = 0;
				POUR (virgule->expressions) {
					auto it_orig = nvirgule->expressions[index]->comme_reference_declaration();
					it->comme_reference_declaration()->declaration_referee = copie_noeud(assem, it_orig->declaration_referee, bloc_parent)->comme_declaration_variable();
				}
			}

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_COMME);
			auto expr = racine->comme_comme();
			auto nexpr = nracine->comme_comme();
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->expression_type = copie_noeud(assem, expr->expression_type, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE);
			auto expr = racine->comme_assignation_variable();
			auto nexpr = nracine->comme_assignation_variable();

			nexpr->variable = copie_noeud(assem, expr->variable, bloc_parent);
			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_INDEXAGE);
			auto expr = static_cast<NoeudExpressionBinaire const *>(racine);
			auto nexpr = static_cast<NoeudExpressionBinaire *>(nracine);

			nexpr->operande_gauche = copie_noeud(assem, expr->operande_gauche, bloc_parent);
			nexpr->operande_droite = copie_noeud(assem, expr->operande_droite, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE);
			auto expr = static_cast<NoeudExpressionMembre const *>(racine);
			auto nexpr = static_cast<NoeudExpressionMembre *>(nracine);

			nexpr->accedee = copie_noeud(assem, expr->accedee, bloc_parent);
			nexpr->membre = copie_noeud(assem, expr->membre, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE);
			auto expr = static_cast<NoeudExpressionAppel const *>(racine);
			auto nexpr = static_cast<NoeudExpressionAppel *>(nracine);

			nexpr->appelee = copie_noeud(assem, expr->appelee, bloc_parent);

			nexpr->parametres.reserve(expr->parametres.taille());

			POUR (expr->parametres) {
				nexpr->parametres.ajoute(copie_noeud(assem, it, bloc_parent));
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU);
			auto expr = static_cast<NoeudExpressionUnaire const *>(racine);
			auto nexpr = static_cast<NoeudExpressionUnaire *>(nracine);

			nexpr->operande = copie_noeud(assem, expr->operande, bloc_parent);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_RETOUR);
			auto inst = racine->comme_retourne();
			auto ninst = nracine->comme_retourne();
			ninst->expression = copie_noeud(assem, inst->expression, bloc_parent);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_RETIENS);
			auto inst = racine->comme_retiens();
			auto ninst = nracine->comme_retiens();
			ninst->expression = copie_noeud(assem, inst->expression, bloc_parent);
			break;
		}
		case GenreNoeud::DIRECTIVE_CUISINE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DIRECTIVE_CUISINE);
			auto cuisine = racine->comme_cuisine();
			auto ncuisine = nracine->comme_cuisine();
			ncuisine->operande = copie_noeud(assem, cuisine->operande, bloc_parent);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DIRECTIVE_EXECUTE);
			auto expr = static_cast<NoeudDirectiveExecute const *>(racine);
			auto nexpr = static_cast<NoeudDirectiveExecute *>(nracine);

			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN);
			auto lit = racine->comme_litterale();
			auto nlit = nracine->comme_litterale();
			nlit->valeur_entiere = lit->valeur_entiere;
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION);
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES);
			auto expr = static_cast<NoeudTableauArgsVariadiques const *>(racine);
			auto nexpr = static_cast<NoeudTableauArgsVariadiques *>(nracine);
			nexpr->expressions.reserve(expr->expressions.taille());

			POUR (expr->expressions) {
				nexpr->expressions.ajoute(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_BOUCLE);
			auto expr = static_cast<NoeudBoucle const *>(racine);
			auto nexpr = static_cast<NoeudBoucle *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc	*>(copie_noeud(assem, expr->bloc, bloc_parent));
			nexpr->bloc->appartiens_a_boucle = nexpr;
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_POUR);
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_DISCR);
			auto expr = static_cast<NoeudDiscr const *>(racine);
			auto nexpr = static_cast<NoeudDiscr *>(nracine);
			nexpr->paires_discr.reserve(expr->paires_discr.taille());

			nexpr->expression_discriminee = copie_noeud(assem, expr->expression_discriminee, bloc_parent);
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
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_SAUFSI);
			auto expr = static_cast<NoeudSi const *>(racine);
			auto nexpr = static_cast<NoeudSi *>(nracine);

			nexpr->condition = copie_noeud(assem, expr->condition, bloc_parent);
			nexpr->bloc_si_vrai = copie_noeud(assem, expr->bloc_si_vrai, bloc_parent);
			nexpr->bloc_si_faux = copie_noeud(assem, expr->bloc_si_faux, bloc_parent);
			break;
		}
		case GenreNoeud::INSTRUCTION_SI_STATIQUE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_SI_STATIQUE);
			auto inst = racine->comme_si_statique();
			auto ninst = nracine->comme_si_statique();
			ninst->condition = copie_noeud(assem, inst->condition, bloc_parent);
			ninst->bloc_si_vrai = static_cast<NoeudBloc	*>(copie_noeud(assem, inst->bloc_si_vrai, bloc_parent));
			ninst->bloc_si_faux = static_cast<NoeudBloc	*>(copie_noeud(assem, inst->bloc_si_faux, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE);
			auto expr = static_cast<NoeudPousseContexte const *>(racine);
			auto nexpr = static_cast<NoeudPousseContexte *>(nracine);

			nexpr->expression = copie_noeud(assem, expr->expression, bloc_parent);
			nexpr->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr->bloc, bloc_parent));

			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_TENTE);
			auto inst = static_cast<NoeudInstructionTente const *>(racine);
			auto ninst = static_cast<NoeudInstructionTente *>(nracine);

			ninst->expression_appelee = copie_noeud(assem, inst->expression_appelee, bloc_parent);
			ninst->expression_piegee = copie_noeud(assem, inst->expression_piegee, bloc_parent);
			ninst->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, inst->bloc, bloc_parent));
			break;
		}
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::INSTRUCTION_NON_INITIALISATION);
			break;
		}
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::EXPRESSION_VIRGULE);
			auto expr = racine->comme_virgule();
			auto nexpr = nracine->comme_virgule();

			nexpr->expressions.reserve(expr->expressions.taille());

			POUR (expr->expressions) {
				nexpr->expressions.ajoute(copie_noeud(assem, it, bloc_parent));
			}

			break;
		}
		case GenreNoeud::DECLARATION_MODULE:
		{
			CREE_NOEUD_POUR_COPIE(GenreNoeud::DECLARATION_MODULE);
			auto expr = racine->comme_declaration_module();
			auto nexpr = nracine->comme_declaration_module();
			nexpr->module = expr->module;
			break;
		}
	}

	return nracine;
}

Etendue calcule_etendue_noeud(const NoeudExpression *racine)
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
		case GenreNoeud::DECLARATION_MODULE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto expr = racine->comme_declaration_variable();

			auto etendue_enfant = calcule_etendue_noeud(expr->valeur);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto expr = racine->comme_comme();

			auto etendue_enfant = calcule_etendue_noeud(expr->expression);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression_type);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr = racine->comme_assignation_variable();

			auto etendue_enfant = calcule_etendue_noeud(expr->variable);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->expression);
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

			auto etendue_enfant = calcule_etendue_noeud(expr->operande_gauche);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->operande_droite);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto expr = static_cast<NoeudExpressionMembre const *>(racine);

			auto etendue_enfant = calcule_etendue_noeud(expr->accedee);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			etendue_enfant = calcule_etendue_noeud(expr->membre);

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

			auto etendue_enfant = calcule_etendue_noeud(expr->operande);

			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);

			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			auto inst = racine->comme_retourne();
			auto etendue_enfant = calcule_etendue_noeud(inst->expression);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			auto inst = racine->comme_retiens();
			auto etendue_enfant = calcule_etendue_noeud(inst->expression);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTE:
		{
			auto dir = static_cast<NoeudDirectiveExecute const *>(racine);
			auto etendue_enfant = calcule_etendue_noeud(dir->expression);
			etendue.pos_min = std::min(etendue.pos_min, etendue_enfant.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_enfant.pos_max);
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL:
		{
			auto expr = static_cast<NoeudExpressionAppel const *>(racine);

			auto etendue_expr = calcule_etendue_noeud(expr->appelee);

			etendue.pos_min = std::min(etendue.pos_min, etendue_expr.pos_min);
			etendue.pos_max = std::max(etendue.pos_max, etendue_expr.pos_max);

			POUR (expr->parametres) {
				auto etendue_enfant = calcule_etendue_noeud(it);

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

			auto etendue_enfant = calcule_etendue_noeud(expr->operande);

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
				auto etendue_enfant = calcule_etendue_noeud(it);
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

kuri::chaine const &NoeudDeclarationEnteteFonction::nom_broye(EspaceDeTravail *espace)
{
	if (nom_broye_ != "") {
		return nom_broye_;
	}

	if (ident != ID::principale && !possede_drapeau(EST_EXTERNE | FORCE_SANSBROYAGE)) {
		auto fichier = espace->fichier(lexeme->fichier);

		if (est_metaprogramme) {
			nom_broye_ = enchaine("metaprogramme", this);
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
