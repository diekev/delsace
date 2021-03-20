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
	auto bloc = static_cast<NoeudBloc *>(cree_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme));
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

NoeudExpressionUnaire *AssembleuseArbre::cree_importe(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_IMPORTE>(lexeme)->comme_importe();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_charge(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_CHARGE>(lexeme)->comme_charge();
}

NoeudExpressionVirgule *AssembleuseArbre::cree_virgule(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_VIRGULE>(lexeme)->comme_virgule();
}

NoeudBoucle *AssembleuseArbre::cree_boucle(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_BOUCLE>(lexeme)->comme_boucle();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_op_binaire(Lexeme const *lexeme)
{
	return cree_noeud<GenreNoeud::OPERATEUR_BINAIRE>(lexeme)->comme_operateur_binaire();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_op_binaire(const Lexeme *lexeme, const OperateurBinaire *op, NoeudExpression *expr1, NoeudExpression *expr2)
{
	assert(op);
	auto op_bin = cree_op_binaire(lexeme);
	op_bin->operande_gauche = expr1;
	op_bin->operande_droite = expr2;
	op_bin->op = op;
	op_bin->type = op->type_resultat;
	return op_bin;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DECLARATION_VARIABLE>(lexeme)->comme_declaration_variable();
}

NoeudExpressionReference *AssembleuseArbre::cree_reference_declaration(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_REFERENCE_DECLARATION>(lexeme)->comme_reference_declaration();
}

NoeudExpressionReference *AssembleuseArbre::cree_reference_declaration(const Lexeme *lexeme, NoeudDeclaration *decl)
{
	auto ref = cree_reference_declaration(lexeme);
	ref->declaration_referee = decl;
	ref->type = decl->type;
	ref->ident = decl->ident;
	return ref;
}

NoeudSi *AssembleuseArbre::cree_si(const Lexeme *lexeme, GenreNoeud genre_noeud)
{
	if (genre_noeud == GenreNoeud::INSTRUCTION_SI) {
		return static_cast<NoeudSi *>(cree_noeud<GenreNoeud::INSTRUCTION_SI>(lexeme));
	}

	return static_cast<NoeudSi *>(cree_noeud<GenreNoeud::INSTRUCTION_SAUFSI>(lexeme));
}

NoeudBloc *AssembleuseArbre::cree_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent)
{
	auto bloc = cree_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme)->comme_bloc();
	bloc->bloc_parent = bloc_parent;
	return bloc;
}

NoeudExpression *AssembleuseArbre::cree_arrete(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_CONTINUE_ARRETE>(lexeme)->comme_controle_boucle();
}

NoeudExpression *AssembleuseArbre::cree_continue(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_CONTINUE_ARRETE>(lexeme)->comme_controle_boucle();
}

NoeudAssignation *AssembleuseArbre::cree_assignation(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE>(lexeme)->comme_assignation_variable();
}

NoeudAssignation *AssembleuseArbre::cree_assignation(const Lexeme *lexeme, NoeudExpression *assignee, NoeudExpression *expression)
{
	auto assignation = cree_noeud<GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE>(lexeme)->comme_assignation_variable();

	auto donnees = DonneesAssignations();
	donnees.expression = expression;
	donnees.variables.ajoute(assignee);
	donnees.transformations.ajoute({});

	assignation->donnees_exprs.ajoute(donnees);

	return assignation;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(const Lexeme *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression)
{
	auto ref = cree_reference_declaration(lexeme);
	ref->ident = ident;
	ref->type = type;
	return cree_declaration_variable(ref, expression);
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(NoeudExpressionReference *ref, NoeudExpression *expression)
{
	auto declaration = cree_declaration_variable(ref->lexeme);
	declaration->ident = ref->ident;
	declaration->type = ref->type;
	declaration->valeur = ref;
	declaration->expression = expression;

	ref->declaration_referee = declaration;

	auto donnees = DonneesAssignations();
	donnees.expression = expression;
	donnees.variables.ajoute(ref);
	donnees.transformations.ajoute({});

	declaration->donnees_decl.ajoute(donnees);

	return declaration;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(NoeudExpressionReference *ref)
{
	auto decl = cree_declaration_variable(ref->lexeme);
	decl->valeur = ref;
	decl->ident = ref->ident;
	ref->declaration_referee = decl;
	return decl;
}

NoeudExpressionMembre *AssembleuseArbre::cree_acces_membre(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_REFERENCE_MEMBRE>(lexeme)->comme_reference_membre();
}

NoeudExpressionMembre *AssembleuseArbre::cree_acces_membre(const Lexeme *lexeme, NoeudExpression *accede, Type *type, int index)
{
	auto acces = cree_acces_membre(lexeme);
	acces->accedee = accede;
	acces->type = type;
	acces->index_membre = index;
	return acces;
}

NoeudExpressionUnaire *AssembleuseArbre::cree_op_unaire(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::OPERATEUR_UNAIRE>(lexeme)->comme_operateur_unaire();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_indexage(const Lexeme *lexeme)
{
	auto indexage = cree_noeud<GenreNoeud::EXPRESSION_INDEXAGE>(lexeme)->comme_indexage();
	return indexage;
}

NoeudExpressionBinaire *AssembleuseArbre::cree_indexage(const Lexeme *lexeme, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification)
{
	auto indexage = cree_noeud<GenreNoeud::EXPRESSION_INDEXAGE>(lexeme)->comme_indexage();
	indexage->operande_gauche = expr1;
	indexage->operande_droite = expr2;
	indexage->type = type_dereference_pour(expr1->type);
	if (ignore_verification) {
		indexage->aide_generation_code = IGNORE_VERIFICATION;
	}
	return indexage;
}

NoeudExpressionAppel *AssembleuseArbre::cree_appel(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_APPEL>(lexeme)->comme_appel();
}

NoeudExpressionAppel *AssembleuseArbre::cree_appel(const Lexeme *lexeme, NoeudExpression *appelee, Type *type)
{
	auto appel = cree_appel(lexeme);
	appel->noeud_fonction_appelee =	appelee;
	appel->type = type;

	if (appelee->est_entete_fonction()) {
		appel->appelee = cree_reference_declaration(lexeme, appelee->comme_entete_fonction());
	}
	else {
		appel->appelee = appelee;
	}

	return appel;
}

NoeudExpressionAppel *AssembleuseArbre::cree_construction_structure(const Lexeme *lexeme, TypeCompose *type)
{
	auto structure = cree_appel(lexeme);
	structure->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
	structure->parametres_resolus.reserve(type->membres.taille());

	if (type->est_structure()) {
		structure->appelee = type->comme_structure()->decl;
		structure->noeud_fonction_appelee = type->comme_structure()->decl;
	}
	else if (type->est_union()) {
		structure->appelee = type->comme_union()->decl;
		structure->noeud_fonction_appelee = type->comme_union()->decl;
	}

	structure->type = type;
	return structure;
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_chaine(const Lexeme *lexeme)
{
	auto noeud = cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_CHAINE>(lexeme)->comme_litterale();
	/* transfère l'index car les lexèmes peuvent être partagés lors de la simplification du code ou des exécutions */
	noeud->index_chaine = lexeme->index_chaine;
	return noeud;
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_caractere(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_CARACTERE>(lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_entier(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER>(lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_reel(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL>(lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_nul(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_NUL>(lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_bool(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN>(lexeme)->comme_litterale();
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_entier(Lexeme const *lexeme, Type *type, unsigned long valeur)
{
	auto lit = cree_litterale_entier(lexeme);
	lit->type = type;
	lit->valeur_entiere = valeur;
	return lit;
}

NoeudExpressionLitterale *AssembleuseArbre::cree_litterale_reel(Lexeme const *lexeme, Type *type, double valeur)
{
	auto lit = cree_litterale_reel(lexeme);
	lit->type = type;
	lit->valeur_reelle = valeur;
	return lit;
}

NoeudExpression *AssembleuseArbre::cree_reference_type(Lexeme const *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_REFERENCE_TYPE>(lexeme);
}

NoeudExpression *AssembleuseArbre::cree_reference_type(Lexeme const *lexeme, Type *type)
{
	auto ref_type = cree_reference_type(lexeme);
	ref_type->type = type;
	return ref_type;
}

NoeudAssignation *AssembleuseArbre::cree_incrementation(const Lexeme *lexeme, NoeudExpression *valeur)
{
	auto type = valeur->type;

	auto inc = cree_op_binaire(lexeme);
	inc->op = type->operateur_ajt;
	assert(inc->op);
	inc->operande_gauche = valeur;
	inc->type = type;

	if (est_type_entier(type)) {
		inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
	}
	else if (type->est_reel()) {
		// À FAIRE(r16)
		inc->operande_droite = cree_litterale_reel(valeur->lexeme, type, 1.0);
	}

	return cree_assignation(valeur->lexeme, valeur, inc);
}

NoeudAssignation *AssembleuseArbre::cree_decrementation(const Lexeme *lexeme, NoeudExpression *valeur)
{
	auto type = valeur->type;

	auto inc = cree_op_binaire(lexeme);
	inc->op = type->operateur_sst;
	assert(inc->op);
	inc->operande_gauche = valeur;
	inc->type = type;

	if (est_type_entier(type)) {
		inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
	}
	else if (type->est_reel()) {
		// À FAIRE(r16)
		inc->operande_droite = cree_litterale_reel(valeur->lexeme, type, 1.0);
	}

	return cree_assignation(valeur->lexeme, valeur, inc);
}

NoeudPour *AssembleuseArbre::cree_pour(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_POUR>(lexeme)->comme_pour();
}

NoeudBoucle *AssembleuseArbre::cree_repete(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_REPETE>(lexeme)->comme_repete();
}

NoeudBoucle *AssembleuseArbre::cree_tantque(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_TANTQUE>(lexeme)->comme_tantque();
}

NoeudSiStatique *AssembleuseArbre::cree_si_statique(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_SI_STATIQUE>(lexeme)->comme_si_statique();
}

NoeudDiscr *AssembleuseArbre::cree_discr(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_DISCR>(lexeme)->comme_discr();
}

NoeudEnum *AssembleuseArbre::cree_enum(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DECLARATION_ENUM>(lexeme)->comme_enum();
}

NoeudStruct *AssembleuseArbre::cree_struct(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DECLARATION_STRUCTURE>(lexeme)->comme_structure();
}

NoeudTableauArgsVariadiques *AssembleuseArbre::cree_tableau_variadique(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES>(lexeme)->comme_args_variadiques();
}

NoeudDeclarationEnteteFonction *AssembleuseArbre::cree_entete_fonction(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DECLARATION_ENTETE_FONCTION>(lexeme)->comme_entete_fonction();
}

NoeudRetour *AssembleuseArbre::cree_retour(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_RETOUR>(lexeme)->comme_retour();
}

NoeudRetour *AssembleuseArbre::cree_retiens(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_RETIENS>(lexeme)->comme_retiens();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_controle_boucle(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_CONTINUE_ARRETE>(lexeme)->comme_controle_boucle();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_cuisine(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DIRECTIVE_CUISINE>(lexeme)->comme_cuisine();
}

NoeudComme *AssembleuseArbre::cree_comme(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_COMME>(lexeme)->comme_comme();
}

NoeudExpressionBinaire *AssembleuseArbre::cree_plage(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_PLAGE>(lexeme)->comme_plage();
}

NoeudDirectiveExecute *AssembleuseArbre::cree_execution(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::DIRECTIVE_EXECUTE>(lexeme)->comme_execute();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_type_de(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_TYPE_DE>(lexeme)->comme_type_de();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_taille_de(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_TAILLE_DE>(lexeme)->comme_taille();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_info_de(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_INFO_DE>(lexeme)->comme_info_de();
}

NoeudTente *AssembleuseArbre::cree_tente(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_TENTE>(lexeme)->comme_tente();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_parenthese(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_PARENTHESE>(lexeme)->comme_parenthese();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_memoire(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_MEMOIRE>(lexeme)->comme_memoire();
}

NoeudExpression *AssembleuseArbre::cree_non_initialisation(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_NON_INITIALISATION>(lexeme);
}

NoeudExpressionUnaire *AssembleuseArbre::cree_init_de(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_INIT_DE>(lexeme)->comme_init_de();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_empl(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_EMPL>(lexeme)->comme_empl();
}

NoeudExpressionUnaire *AssembleuseArbre::cree_construction_tableau(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU>(lexeme)->comme_construction_tableau();
}

NoeudPousseContexte *AssembleuseArbre::cree_pousse_contexte(const Lexeme *lexeme)
{
	return cree_noeud<GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE>(lexeme)->comme_pousse_contexte();
}
