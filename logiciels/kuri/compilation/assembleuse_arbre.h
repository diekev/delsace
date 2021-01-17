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

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"

enum class GenreNoeud : char;

struct AllocatriceNoeud;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Lexeme;
struct NoeudAssignation;
struct NoeudBloc;
struct NoeudBoucle;
struct NoeudComme;
struct NoeudDeclaration;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudDirectiveExecution;
struct NoeudDiscr;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudExpressionBinaire;
struct NoeudExpressionLitterale;
struct NoeudExpressionMembre;
struct NoeudExpressionReference;
struct NoeudExpressionUnaire;
struct NoeudExpressionVirgule;
struct NoeudPour;
struct NoeudPousseContexte;
struct NoeudRetour;
struct NoeudSi;
struct NoeudSiStatique;
struct NoeudStruct;
struct NoeudTableauArgsVariadiques;
struct NoeudTente;
struct OperateurBinaire;
struct Type;

struct AssembleuseArbre {
private:
	AllocatriceNoeud &m_allocatrice_noeud;

	size_t m_memoire_utilisee = 0;

	dls::pile<NoeudBloc *> m_blocs{};

public:
	explicit AssembleuseArbre(AllocatriceNoeud &allocatrice);
	~AssembleuseArbre() = default;

	NoeudBloc *empile_bloc(Lexeme const *lexeme);

	NoeudBloc *bloc_courant() const;

	void bloc_courant(NoeudBloc *bloc);

	void depile_tout();

	void depile_bloc();

	/**
	 * Crée un noeud sans le désigner comme noeud courant, et retourne un
	 * pointeur vers celui-ci.
	 */
	NoeudExpression *cree_noeud(GenreNoeud type, Lexeme const *lexeme);

	NoeudAssignation *cree_assignation(const Lexeme *lexeme);
	NoeudAssignation *cree_assignation(const Lexeme *lexeme, NoeudExpression *assignee, NoeudExpression *expression);
	NoeudAssignation *cree_incrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudAssignation *cree_decrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudBloc *cree_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent);
	NoeudBoucle *cree_boucle(Lexeme const *lexeme);
	NoeudBoucle *cree_repete(const Lexeme *lexeme);
	NoeudBoucle *cree_tantque(const Lexeme *lexeme);
	NoeudComme *cree_comme(const Lexeme *lexeme);
	NoeudDeclarationEnteteFonction *cree_entete_fonction(const Lexeme *lexeme);
	NoeudDeclarationVariable *cree_declaration(Lexeme const *lexeme);
	NoeudDeclarationVariable *cree_declaration(const Lexeme *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression);
	NoeudDeclarationVariable *cree_declaration(NoeudExpressionReference *ref, NoeudExpression *expression);
	NoeudDirectiveExecution *cree_execution(const Lexeme *lexeme);
	NoeudDiscr *cree_discr(const Lexeme *lexeme);
	NoeudEnum *cree_enum(const Lexeme *lexeme);
	NoeudExpression *cree_arrete(const Lexeme *lexeme);
	NoeudExpression *cree_continue(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_bool(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_caractere(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_chaine(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_entier(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_entier(const Lexeme *lexeme, Type *type, unsigned long valeur);
	NoeudExpressionLitterale *cree_lit_nul(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_reel(const Lexeme *lexeme);
	NoeudExpressionLitterale *cree_lit_reel(const Lexeme *lexeme, Type *type, double valeur);
	NoeudExpression *cree_non_initialisation(const Lexeme *lexeme);
	NoeudExpression *cree_ref_type(const Lexeme *lexeme);
	NoeudExpression *cree_ref_type(const Lexeme *lexeme, Type *type);
	NoeudExpressionAppel *cree_appel(const Lexeme *lexeme);
	NoeudExpressionAppel *cree_appel(const Lexeme *lexeme, NoeudExpression *appelee, Type *type);
	NoeudExpressionBinaire *cree_indexage(const Lexeme *lexeme);
	NoeudExpressionBinaire *cree_indexage(const Lexeme *lexeme, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification);
	NoeudExpressionBinaire *cree_op_binaire(const Lexeme *lexeme);
	NoeudExpressionBinaire *cree_op_binaire(const Lexeme *lexeme, OperateurBinaire const *op, NoeudExpression *expr1, NoeudExpression *expr2);
	NoeudExpressionBinaire *cree_plage(const Lexeme *lexeme);
	NoeudExpressionMembre *cree_acces_membre(const Lexeme *lexeme);
	NoeudExpressionMembre *cree_acces_membre(const Lexeme *lexeme, NoeudExpression *accede, Type *type, int index);
	NoeudExpressionReference *cree_ref_decl(const Lexeme *lexeme);
	NoeudExpressionReference *cree_ref_decl(const Lexeme *lexeme, NoeudDeclaration *decl);
	NoeudExpressionUnaire *cree_charge(Lexeme const *lexeme);
	NoeudExpressionUnaire *cree_construction_tableau(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_controle_boucle(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_cuisine(Lexeme const *lexeme);
	NoeudExpressionUnaire *cree_empl(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_importe(Lexeme const *lexeme);
	NoeudExpressionUnaire *cree_info_de(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_init_de(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_memoire(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_op_unaire(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_parenthese(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_taille_de(const Lexeme *lexeme);
	NoeudExpressionUnaire *cree_type_de(const Lexeme *lexeme);
	NoeudExpressionVirgule *cree_virgule(Lexeme const *lexeme);
	NoeudPour *cree_pour(const Lexeme *lexeme);
	NoeudPousseContexte *cree_pousse_contexte(const Lexeme *lexeme);
	NoeudRetour *cree_retiens(const Lexeme *lexeme);
	NoeudRetour *cree_retour(const Lexeme *lexeme);
	NoeudSi *cree_si(const Lexeme *lexeme, GenreNoeud genre_noeud);
	NoeudSiStatique *cree_si_statique(const Lexeme *lexeme);
	NoeudStruct *cree_struct(const Lexeme *lexeme);
	NoeudTableauArgsVariadiques *cree_tableau_variadique(const Lexeme *lexeme);
	NoeudTente *cree_tente(const Lexeme *lexeme);
};
