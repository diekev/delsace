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

#include "allocatrice_noeud.hh"
#include "operateurs.hh"

AssembleuseArbre::AssembleuseArbre(AllocatriceNoeud &allocatrice)
	: m_allocatrice_noeud(allocatrice)
{
}

NoeudBloc *AssembleuseArbre::empile_bloc(Lexeme const *lexeme)
{
	auto bloc = static_cast<NoeudBloc *>(cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, lexeme));
	bloc->bloc_parent = bloc_courant();

	if (bloc->bloc_parent) {
		bloc->possede_contexte = bloc->bloc_parent->possede_contexte;
	}
	else {
		/* vrai si le bloc ne possède pas de parent (bloc de module) */
		bloc->possede_contexte = true;
	}

	m_blocs.empile(bloc);

	return bloc;
}

NoeudBloc *AssembleuseArbre::bloc_courant() const
{
	if (m_blocs.est_vide()) {
		return nullptr;
	}

	return m_blocs.haut();
}

void AssembleuseArbre::bloc_courant(NoeudBloc *bloc)
{
	m_blocs.empile(bloc);
}

void AssembleuseArbre::depile_tout()
{
	m_blocs.efface();
}

void AssembleuseArbre::depile_bloc()
{
	m_blocs.depile();
}

NoeudExpression *AssembleuseArbre::cree_noeud(GenreNoeud genre, Lexeme const *lexeme)
{
	auto noeud = m_allocatrice_noeud.cree_noeud(genre);
	noeud->genre = genre;
	noeud->lexeme = lexeme;
	noeud->bloc_parent = bloc_courant();

	if (noeud->lexeme && (noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE || noeud->lexeme->genre == GenreLexeme::EXTERNE)) {
		noeud->ident = lexeme->ident;
	}

	if (genre == GenreNoeud::DECLARATION_ENTETE_FONCTION) {
		auto entete = noeud->comme_entete_fonction();
		entete->corps->lexeme = lexeme;
		entete->corps->ident = lexeme->ident;
		entete->corps->bloc_parent = entete->bloc_parent;
	}

	return noeud;
}

NoeudExpressionUnaire *AssembleuseArbre::cree_importe(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_IMPORTE, lexeme)->comme_importe();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_charge(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_CHARGE, lexeme)->comme_charge();
}

NoeudExpressionVirgule *AssembleuseArbre::cree_virgule(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_VIRGULE, lexeme)->comme_virgule();
}

NoeudBoucle *AssembleuseArbre::cree_boucle(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_BOUCLE, lexeme)->comme_boucle();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_op_binaire(Lexeme const *lexeme)
{
	return cree_noeud(GenreNoeud::OPERATEUR_BINAIRE, lexeme)->comme_operateur_binaire();
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::DECLARATION_VARIABLE, lexeme)->comme_decl_var();
}

NoeudExpressionReference *AssembleuseArbre::cree_ref_decl(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION, lexeme)->comme_ref_decl();
}

NoeudExpressionReference *AssembleuseArbre::cree_ref_decl(const Lexeme *lexeme, NoeudDeclaration *decl)
{
	auto ref = cree_ref_decl(lexeme);
	ref->decl = decl;
	ref->type = decl->type;
	ref->ident = decl->ident;
	return ref;
}

NoeudSi *AssembleuseArbre::cree_si(const Lexeme *lexeme, GenreNoeud genre_noeud)
{
	return static_cast<NoeudSi *>(cree_noeud(genre_noeud, lexeme));
}

NoeudBloc *AssembleuseArbre::cree_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent)
{
	auto bloc = cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, lexeme)->comme_bloc();
	bloc->bloc_parent = bloc_parent;
	return bloc;
}

NoeudExpression *AssembleuseArbre::cree_arrete(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, lexeme)->comme_controle_boucle();
}

NoeudExpression *AssembleuseArbre::cree_continue(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, lexeme)->comme_controle_boucle();
}

NoeudAssignation *AssembleuseArbre::cree_assignation(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE, lexeme)->comme_assignation();
}

NoeudAssignation *AssembleuseArbre::cree_assignation(const Lexeme *lexeme, NoeudExpression *assignee, NoeudExpression *expression)
{
	auto assignation = cree_noeud(GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE, lexeme)->comme_assignation();

	auto donnees = DonneesAssignations();
	donnees.expression = expression;
	donnees.variables.ajoute(assignee);
	donnees.transformations.ajoute({});

	assignation->donnees_exprs.ajoute(donnees);

	return assignation;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration(const Lexeme *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression)
{
	auto declaration = cree_declaration(lexeme);
	declaration->ident = ident;
	declaration->type = type;

	auto donnees = DonneesAssignations();
	donnees.expression = expression;
	donnees.variables.ajoute(declaration);
	donnees.transformations.ajoute({});

	declaration->donnees_decl.ajoute(donnees);

	return declaration;
}

NoeudExpressionMembre *AssembleuseArbre::cree_acces_membre(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE, lexeme)->comme_ref_membre();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_op_unaire(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::OPERATEUR_UNAIRE, lexeme)->comme_operateur_unaire();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_indexage(const Lexeme *lexeme)
{
	auto indexage = cree_noeud(GenreNoeud::EXPRESSION_INDEXAGE, lexeme)->comme_indexage();
	return indexage;
}

NoeudExpressionAppel *AssembleuseArbre::cree_appel(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_APPEL_FONCTION, lexeme)->comme_appel();
}

NoeudExpressionAppel *AssembleuseArbre::cree_appel(const Lexeme *lexeme, NoeudExpression *appelee)
{
	auto appel = cree_appel(lexeme);
	appel->noeud_fonction_appelee =	appelee;
	appel->appelee = appelee;
	return appel;
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_chaine(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_CHAINE, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_caractere(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_CARACTERE, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_entier(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_reel(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_nul(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_NUL, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_bool(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN, lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_entier(Lexeme const *lexeme, Type *type, unsigned long valeur)
{
	auto lit = cree_lit_entier(lexeme);
	lit->type = type;
	lit->valeur_entiere = valeur;
	return lit;
}

NoeudExpressionLitterale *AssembleuseArbre::cree_lit_reel(Lexeme const *lexeme, Type *type, double valeur)
{
	auto lit = cree_lit_reel(lexeme);
	lit->type = type;
	lit->valeur_reelle = valeur;
	return lit;
}

NoeudExpression *AssembleuseArbre::cree_ref_type(Lexeme const *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_REFERENCE_TYPE, lexeme);
}

NoeudExpression *AssembleuseArbre::cree_ref_type(Lexeme const *lexeme, Type *type)
{
	auto ref_type = cree_ref_type(lexeme);
	ref_type->type = type;
	return ref_type;
}

NoeudAssignation *AssembleuseArbre::cree_incrementation(const Lexeme *lexeme, NoeudExpression *valeur)
{
	auto type = valeur->type;

	auto inc = cree_op_binaire(lexeme);
	inc->op = type->operateur_ajt;
	assert(inc->op);
	inc->expr1 = valeur;
	inc->type = type;

	if (est_type_entier(type)) {
		inc->expr2 = cree_lit_entier(valeur->lexeme, type, 1);
	}
	else if (type->est_reel()) {
		// À FAIRE : r16
		inc->expr2 = cree_lit_reel(valeur->lexeme, type, 1.0);
	}

	return cree_assignation(valeur->lexeme, valeur, inc);
}

NoeudPour *AssembleuseArbre::cree_pour(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_POUR, lexeme)->comme_pour();
}

NoeudBoucle *AssembleuseArbre::cree_repete(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_REPETE, lexeme)->comme_repete();
}

NoeudBoucle *AssembleuseArbre::cree_tantque(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_TANTQUE, lexeme)->comme_tantque();
}

NoeudSiStatique *AssembleuseArbre::cree_si_statique(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_SI_STATIQUE, lexeme)->comme_si_statique();
}

NoeudDiscr *AssembleuseArbre::cree_discr(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_DISCR, lexeme)->comme_discr();
}

NoeudEnum *AssembleuseArbre::cree_enum(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::DECLARATION_ENUM, lexeme)->comme_enum();
}

NoeudStruct *AssembleuseArbre::cree_struct(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::DECLARATION_STRUCTURE, lexeme)->comme_structure();
}

NoeudTableauArgsVariadiques *AssembleuseArbre::cree_tableau_variadique(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES, lexeme)->comme_args_variadiques();
}

NoeudDeclarationEnteteFonction *AssembleuseArbre::cree_entete_fonction(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::DECLARATION_ENTETE_FONCTION, lexeme)->comme_entete_fonction();
}

NoeudRetour *AssembleuseArbre::cree_retour(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_RETOUR, lexeme)->comme_retour();
}

NoeudRetour *AssembleuseArbre::cree_retiens(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_RETIENS, lexeme)->comme_retiens();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_controle_boucle(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE, lexeme)->comme_controle_boucle();
}

NoeudComme *AssembleuseArbre::cree_comme(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_COMME, lexeme)->comme_comme();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_plage(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_PLAGE, lexeme)->comme_plage();
}

NoeudDirectiveExecution *AssembleuseArbre::cree_execution(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::DIRECTIVE_EXECUTION, lexeme)->comme_execute();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_type_de(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_TYPE_DE, lexeme)->comme_type_de();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_taille_de(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_TAILLE_DE, lexeme)->comme_taille();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_info_de(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_INFO_DE, lexeme)->comme_info_de();
}

NoeudTente *AssembleuseArbre::cree_tente(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_TENTE, lexeme)->comme_tente();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_parenthese(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_PARENTHESE, lexeme)->comme_parenthese();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_memoire(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_MEMOIRE, lexeme)->comme_memoire();
}

NoeudExpression *AssembleuseArbre::cree_non_initialisation(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_NON_INITIALISATION, lexeme);
}

NoeudExpressionUnaire *AssembleuseArbre::cree_init_de(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_INIT_DE, lexeme)->comme_init_de();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_empl(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_EMPL, lexeme)->comme_empl();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_construction_tableau(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU, lexeme)->comme_construction_tableau();
}

NoeudPousseContexte *AssembleuseArbre::cree_pousse_contexte(const Lexeme *lexeme)
{
	return cree_noeud(GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE, lexeme)->comme_pousse_contexte();
}
