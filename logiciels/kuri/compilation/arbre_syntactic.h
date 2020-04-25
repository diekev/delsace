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

#include <any>

#include "biblinternes/outils/definitions.h"

#include "expression.h"
#include "lexemes.hh"
#include "structures.hh"
#include "typage.hh"
#include "transformation_type.hh"

class assembleuse_arbre;
struct OperateurBinaire;
struct OperateurUnaire;
struct Fichier;
struct IdentifiantCode;
struct NoeudBloc;
struct TypeFonction;

enum class GenreNoeud : char {
	DECLARATION_COROUTINE,
	DECLARATION_ENUM,
	DECLARATION_FONCTION,
	DECLARATION_OPERATEUR,
	DECLARATION_STRUCTURE,
	DECLARATION_VARIABLE,
	DIRECTIVE_EXECUTION,
	EXPRESSION_APPEL_FONCTION,
	EXPRESSION_ASSIGNATION_VARIABLE,
	EXPRESSION_CONSTRUCTION_STRUCTURE,
	EXPRESSION_CONSTRUCTION_TABLEAU,
	EXPRESSION_DELOGE,
	EXPRESSION_INDEXAGE,
	EXPRESSION_INFO_DE,
	EXPRESSION_LITTERALE_BOOLEEN,
	EXPRESSION_LITTERALE_CARACTERE,
	EXPRESSION_LITTERALE_CHAINE,
	EXPRESSION_LITTERALE_NOMBRE_REEL,
	EXPRESSION_LITTERALE_NOMBRE_ENTIER,
	EXPRESSION_LITTERALE_NUL,
	EXPRESSION_LOGE,
	EXPRESSION_MEMOIRE,
	EXPRESSION_PARENTHESE,
	EXPRESSION_PLAGE,
	EXPRESSION_REFERENCE_DECLARATION,
	EXPRESSION_REFERENCE_MEMBRE,
	EXPRESSION_REFERENCE_MEMBRE_UNION,
	EXPRESSION_RELOGE,
	EXPRESSION_TABLEAU_ARGS_VARIADIQUES,
	EXPRESSION_TAILLE_DE,
	EXPRESSION_COMME,
	INSTRUCTION_BOUCLE,
	INSTRUCTION_COMPOSEE,
	INSTRUCTION_CONTINUE_ARRETE,
	INSTRUCTION_DISCR,
	INSTRUCTION_DISCR_ENUM,
	INSTRUCTION_DISCR_UNION,
	INSTRUCTION_NON_INITIALISATION,
	INSTRUCTION_POUR,
	INSTRUCTION_POUSSE_CONTEXTE,
	INSTRUCTION_REPETE,
	INSTRUCTION_RETIENS,
	INSTRUCTION_RETOUR,
	INSTRUCTION_RETOUR_MULTIPLE,
	INSTRUCTION_RETOUR_SIMPLE,
	INSTRUCTION_SAUFSI,
	INSTRUCTION_SI,
	INSTRUCTION_SINON,
	INSTRUCTION_TANTQUE,
	INSTRUCTION_TENTE,
	OPERATEUR_BINAIRE,
	OPERATEUR_COMPARAISON_CHAINEE,
	OPERATEUR_UNAIRE,
	EXPANSION_VARIADIQUE,
};

const char *chaine_genre_noeud(GenreNoeud genre);

inline bool est_instruction_retour(GenreNoeud genre)
{
	return genre == GenreNoeud::INSTRUCTION_RETOUR || genre == GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE || genre == GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;
}

bool est_declaration(GenreNoeud genre);

/* ************************************************************************** */

enum drapeaux_noeud : unsigned short {
	AUCUN                      = 0,
	EMPLOYE                    = (1 << 0),
	DECLARATION                = (1 << 1),
	EST_EXTERNE                = (1 << 2),
	EST_CALCULE                = (1 << 3),
	IGNORE_OPERATEUR           = (1 << 4),
	FORCE_ENLIGNE              = (1 << 5),
	FORCE_HORSLIGNE            = (1 << 6),
	FORCE_NULCTX               = (1 << 7),
	FORCE_SANSTRACE            = (1 << 8),
	EST_ASSIGNATION_COMPOSEE   = (1 << 9),
	EST_VARIADIQUE             = (1 << 10),
	EST_VAR_BOUCLE             = (1 << 11),
	EST_GLOBALE                = (1 << 12),
	EST_CONSTANTE              = (1 << 13),
};

DEFINIE_OPERATEURS_DRAPEAU(drapeaux_noeud, unsigned short)

enum {
	/* instruction 'pour' */
	GENERE_BOUCLE_PLAGE,
	GENERE_BOUCLE_PLAGE_INDEX,
	GENERE_BOUCLE_TABLEAU,
	GENERE_BOUCLE_TABLEAU_INDEX,
	GENERE_BOUCLE_COROUTINE,
	GENERE_BOUCLE_COROUTINE_INDEX,

	GENERE_CODE_PTR_FONC_MEMBRE,

	APPEL_POINTEUR_FONCTION,

	APPEL_FONCTION_MOULT_RET,
	APPEL_FONCTION_MOULT_RET2,

	ACCEDE_MODULE,

	/* pour ne pas avoir à générer des conditions de vérification pour par
	 * exemple les accès à des membres d'unions */
	IGNORE_VERIFICATION,

	/* instruction 'retourne' */
	REQUIERS_CODE_EXTRA_RETOUR,
};

/* Le genre d'une valeur, gauche, droite, ou transcendantale.
 *
 * Une valeur gauche est une valeur qui peut être assignée, donc à
 * gauche de '=', et comprend :
 * - les variables et accès de membres de structures
 * - les déréférencements (via mémoire(...))
 * - les opérateurs []
 *
 * Une valeur droite est une valeur qui peut être utilisée dans une
 * assignation, donc à droite de '=', et comprend :
 * - les valeurs littéralles (0, 1.5, "chaine", 'a', vrai)
 * - les énumérations
 * - les variables et accès de membres de structures
 * - les pointeurs de fonctions
 * - les déréférencements (via mémoire(...))
 * - les opérateurs []
 * - les transtypages
 * - les prises d'addresses (via @...)
 *
 * Une valeur transcendantale est une valeur droite qui peut aussi être
 * une valeur gauche (l'intersection des deux ensembles).
 */
enum GenreValeur : char {
	INVALIDE = 0,
	GAUCHE = (1 << 1),
	DROITE = (1 << 2),
	TRANSCENDANTALE = GAUCHE | DROITE,
};

DEFINIE_OPERATEURS_DRAPEAU(GenreValeur, char)

inline bool est_valeur_gauche(GenreValeur type_valeur)
{
	return (type_valeur & GenreValeur::GAUCHE) != GenreValeur::INVALIDE;
}

inline bool est_valeur_droite(GenreValeur type_valeur)
{
	return (type_valeur & GenreValeur::DROITE) != GenreValeur::INVALIDE;
}

struct NoeudBase {
	GenreNoeud genre{};
	GenreValeur genre_valeur{};
	drapeaux_noeud drapeaux = drapeaux_noeud::AUCUN;
	char aide_generation_code = 0;
	REMBOURRE(3);
	Lexeme const *lexeme = nullptr;
	IdentifiantCode *ident = nullptr;
	Type *type = nullptr;

	NoeudBloc *bloc_parent = nullptr;

	DonneesTypeDeclare type_declare{};

	TransformationType transformation{};

	std::any valeur_calculee{};

	/* utilisé pour déterminer les types de retour des fonctions à moultretour
	 * car lors du besoin index_type est utilisé pour le type de retour de la
	 *  première valeur */
	TypeFonction *type_fonc = nullptr;

	NoeudBase() = default;

	COPIE_CONSTRUCT(NoeudBase);

	dls::chaine chaine_calculee() const
	{
		return std::any_cast<dls::chaine>(this->valeur_calculee);
	}
};

/* pour simplifier l'arbre les instructions et déclarations sont également des expressions */
struct NoeudExpression : public NoeudBase {
};

struct NoeudDeclaration : public NoeudExpression {
	drapeaux_noeud drapeaux_decl = {};
};

struct NoeudDeclarationVariable final : public NoeudDeclaration {
	NoeudDeclarationVariable() { genre = GenreNoeud::DECLARATION_VARIABLE; }

	COPIE_CONSTRUCT(NoeudDeclarationVariable);

	// pour une expression de style a := 5, a est la valeur, et 5 l'expression
	// pour une expression de style a, b := foo(7) , « a, b » est la valeur, et foo(7) l'expression
	// la valeur est une peut redondante car ce noeud pourrait contenir les données de la valeur,
	// mais elle est nécessaire pour pouvoir prendre en compte les expressions à virgule
	NoeudExpression *valeur = nullptr;
	NoeudExpression *expression = nullptr;

	ResultatExpression valeur_expression{};
};

struct NoeudExpressionReference : public NoeudExpression {
	NoeudExpressionReference() { genre = GenreNoeud::EXPRESSION_REFERENCE_DECLARATION; }

	NoeudDeclaration *decl = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionReference);
};

struct NoeudExpressionUnaire : public NoeudExpression {
	NoeudExpressionUnaire() {}

	NoeudExpression *expr = nullptr;

	OperateurUnaire const *op = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionUnaire);
};

using NoeudExpressionParenthese = NoeudExpressionUnaire;

struct NoeudExpressionBinaire : public NoeudExpression {
	NoeudExpressionBinaire() {}

	NoeudExpression *expr1 = nullptr;
	NoeudExpression *expr2 = nullptr;

	OperateurBinaire const *op = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionBinaire);
};

struct NoeudExpressionMembre : public NoeudExpression {
	NoeudExpressionMembre() { genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE; }

	NoeudExpression *accede = nullptr;
	NoeudExpression *membre = nullptr;

	int index_membre = 0;

	COPIE_CONSTRUCT(NoeudExpressionMembre);
};

struct NoeudExpressionLogement : public NoeudExpression {
	NoeudExpressionLogement() {}

	NoeudExpression *expr = nullptr;
	NoeudExpression *expr_chaine = nullptr;
	NoeudBloc *bloc = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionLogement);
};

struct NoeudDeclarationFonction : public NoeudDeclaration {
	NoeudDeclarationFonction() { genre = GenreNoeud::DECLARATION_FONCTION; }

	kuri::tableau<NoeudDeclaration *> params{};
	kuri::tableau<NoeudExpression *> arbre_aplatis{};

	kuri::tableau<dls::chaine> noms_retours{};

	NoeudBloc *bloc = nullptr;

	kuri::tableau<dls::vue_chaine_compacte> noms_types_gabarits{};
	bool est_coroutine = false;
	bool est_gabarit = false;
	bool est_variadique = false;
	bool est_externe = false;
	dls::chaine nom_broye = "";

	COPIE_CONSTRUCT(NoeudDeclarationFonction);
};

struct NoeudExpressionAppel : public NoeudExpression {
	NoeudExpressionAppel() { genre = GenreNoeud::EXPRESSION_APPEL_FONCTION; }

	NoeudExpression *appelee = nullptr;

	kuri::tableau<NoeudExpression *> params{};

	kuri::tableau<NoeudExpression *> exprs{};

	NoeudExpression const *noeud_fonction_appelee = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionAppel);
};

struct NoeudStruct : public NoeudDeclaration {
	NoeudStruct() { genre = GenreNoeud::DECLARATION_STRUCTURE; }

	COPIE_CONSTRUCT(NoeudStruct);

	NoeudBloc *bloc = nullptr;

	bool est_union = false;
	bool est_nonsure = false;
	bool est_externe = false;
};

struct NoeudEnum : public NoeudDeclaration {
	NoeudEnum() { genre = GenreNoeud::DECLARATION_STRUCTURE; }

	NoeudBloc *bloc = nullptr;

	COPIE_CONSTRUCT(NoeudEnum);
};

struct NoeudSi : public NoeudExpression {
	NoeudSi() { genre = GenreNoeud::INSTRUCTION_SI; }

	NoeudExpression *condition = nullptr;
	NoeudBloc *bloc_si_vrai = nullptr;
	NoeudBloc *bloc_si_faux = nullptr;

	COPIE_CONSTRUCT(NoeudSi);
};

struct NoeudPour : public NoeudExpression {
	NoeudPour() { genre = GenreNoeud::INSTRUCTION_POUR; }

	NoeudExpression *variable = nullptr;
	NoeudExpression *expression = nullptr;
	NoeudBloc *bloc = nullptr;
	NoeudBloc *bloc_sansarret = nullptr;
	NoeudBloc *bloc_sinon = nullptr;

	COPIE_CONSTRUCT(NoeudPour);
};

struct NoeudBoucle : public NoeudExpression {
	NoeudBoucle() { genre = GenreNoeud::INSTRUCTION_BOUCLE; }

	NoeudExpression *condition = nullptr;
	NoeudBloc *bloc = nullptr;

	COPIE_CONSTRUCT(NoeudBoucle);
};

struct NoeudBloc : public NoeudExpression {
	NoeudBloc() { genre = GenreNoeud::INSTRUCTION_COMPOSEE; }

	NoeudBloc *parent = nullptr;

	NoeudStruct *appartiens_a_struct = nullptr;
	NoeudEnum   *appartiens_a_enum = nullptr;
	NoeudSi     *appartiens_a_boucle = nullptr;
	NoeudStruct *appartiens_a_discr = nullptr;

	kuri::tableau<NoeudDeclaration *> membres{};
	kuri::tableau<NoeudExpression *>  expressions{};

	kuri::tableau<NoeudBloc *> noeuds_differes{};

	bool est_differe = false;
	bool est_nonsur = false;
	bool est_bloc_fonction = false;

	COPIE_CONSTRUCT(NoeudBloc);
};

struct NoeudDiscr : public NoeudExpression {
	NoeudDiscr() { genre = GenreNoeud::INSTRUCTION_DISCR; }

	NoeudExpression *expr = nullptr;

	kuri::tableau<std::pair<NoeudExpression *, NoeudBloc *>> paires_discr{};

	NoeudBloc *bloc_sinon = nullptr;

	OperateurBinaire const *op = nullptr;

	COPIE_CONSTRUCT(NoeudDiscr);
};

struct NoeudPousseContexte : public NoeudExpression {
	NoeudPousseContexte() { genre = GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE; }

	NoeudExpression *expr = nullptr;
	NoeudBloc *bloc = nullptr;

	COPIE_CONSTRUCT(NoeudPousseContexte);
};

struct NoeudTableauArgsVariadiques : public NoeudExpression {
	NoeudTableauArgsVariadiques() { genre = GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES; }

	kuri::tableau<NoeudExpression *> exprs{};

	COPIE_CONSTRUCT(NoeudTableauArgsVariadiques);
};

struct NoeudTente : public NoeudExpression {
	NoeudTente() { genre = GenreNoeud::INSTRUCTION_TENTE; }

	COPIE_CONSTRUCT(NoeudTente);

	NoeudExpression *expr_appel = nullptr;
	NoeudExpression *expr_piege = nullptr;
	NoeudBloc *bloc = nullptr;
};

void imprime_arbre(NoeudBase *racine, std::ostream &os, int tab);

template <unsigned long N>
void rassemble_feuilles(
		NoeudExpression *noeud_base,
		dls::tablet<NoeudExpression *, N> &feuilles)
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

NoeudExpression *copie_noeud(
		assembleuse_arbre *assem,
		NoeudExpression const *racine,
		NoeudBloc *bloc_parent);

void aplatis_arbre(
		NoeudExpression *racine,
		kuri::tableau<NoeudExpression *> &arbre_aplatis);

struct Etendue {
	long pos_min = 0;
	long pos_max = 0;
};

Etendue calcule_etendue_noeud(NoeudExpression *racine, Fichier *fichier);
