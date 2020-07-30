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
struct AtomeFonction;
struct OperateurBinaire;
struct OperateurUnaire;
struct Fichier;
struct IdentifiantCode;
struct NoeudBloc;
struct NoeudBoucle;
struct NoeudCode;
struct NoeudDeclarationVariable;
struct NoeudDirectiveExecution;
struct NoeudDiscr;
struct NoeudEnum;
struct NoeudExpressionAppel;
struct NoeudExpressionBinaire;
struct NoeudExpressionLogement;
struct NoeudExpressionMembre;
struct NoeudExpressionReference;
struct NoeudExpressionUnaire;
struct NoeudPour;
struct NoeudPousseContexte;
struct NoeudSi;
struct NoeudStruct;
struct NoeudTableauArgsVariadiques;
struct NoeudTente;
struct NoeudExpressionAppel;
struct TypeFonction;
struct NoeudDependance;
struct UniteCompilation;

#define ENUMERE_GENRES_NOEUD \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_COROUTINE) \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_ENUM) \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_FONCTION) \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_OPERATEUR) \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_STRUCTURE) \
	ENUMERE_GENRE_NOEUD_EX(DECLARATION_VARIABLE) \
	ENUMERE_GENRE_NOEUD_EX(DIRECTIVE_EXECUTION) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_APPEL_FONCTION) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_ASSIGNATION_VARIABLE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_CONSTRUCTION_STRUCTURE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_CONSTRUCTION_TABLEAU) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_DELOGE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_INDEXAGE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_INFO_DE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_INIT_DE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_BOOLEEN) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_CARACTERE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_CHAINE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_NOMBRE_REEL) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_NOMBRE_ENTIER) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LITTERALE_NUL) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_LOGE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_MEMOIRE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_PARENTHESE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_PLAGE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_REFERENCE_DECLARATION) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_REFERENCE_MEMBRE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_REFERENCE_MEMBRE_UNION) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_REFERENCE_TYPE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_RELOGE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_TABLEAU_ARGS_VARIADIQUES) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_TAILLE_DE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_TYPE_DE) \
	ENUMERE_GENRE_NOEUD_EX(EXPRESSION_COMME) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_BOUCLE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_COMPOSEE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_CONTINUE_ARRETE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_DISCR) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_DISCR_ENUM) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_DISCR_UNION) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_NON_INITIALISATION) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_POUR) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_POUSSE_CONTEXTE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_REPETE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_RETIENS) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_RETOUR) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_RETOUR_MULTIPLE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_RETOUR_SIMPLE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_SAUFSI) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_SI) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_SINON) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_TANTQUE) \
	ENUMERE_GENRE_NOEUD_EX(INSTRUCTION_TENTE) \
	ENUMERE_GENRE_NOEUD_EX(OPERATEUR_BINAIRE) \
	ENUMERE_GENRE_NOEUD_EX(OPERATEUR_COMPARAISON_CHAINEE) \
	ENUMERE_GENRE_NOEUD_EX(OPERATEUR_UNAIRE) \
	ENUMERE_GENRE_NOEUD_EX(EXPANSION_VARIADIQUE)

enum class GenreNoeud : char {
#define ENUMERE_GENRE_NOEUD_EX(genre) genre,
	ENUMERE_GENRES_NOEUD
#undef ENUMERE_GENRE_NOEUD_EX
};

const char *chaine_genre_noeud(GenreNoeud genre);
std::ostream &operator<<(std::ostream &os, GenreNoeud genre);

inline bool est_instruction_retour(GenreNoeud genre)
{
	return genre == GenreNoeud::INSTRUCTION_RETOUR || genre == GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE || genre == GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;
}

bool est_declaration(GenreNoeud genre);

/* ************************************************************************** */

enum drapeaux_noeud : unsigned int {
	AUCUN                      = 0,
	EMPLOYE                    = (1 << 0),
	DECLARATION                = (1 << 1),
	EST_EXTERNE                = (1 << 2),
	IGNORE_OPERATEUR           = (1 << 3),
	FORCE_ENLIGNE              = (1 << 4),
	FORCE_HORSLIGNE            = (1 << 5),
	FORCE_NULCTX               = (1 << 6),
	FORCE_SANSTRACE            = (1 << 7),
	EST_ASSIGNATION_COMPOSEE   = (1 << 8),
	EST_VARIADIQUE             = (1 << 9),
	EST_VAR_BOUCLE             = (1 << 10),
	EST_GLOBALE                = (1 << 11),
	EST_CONSTANTE              = (1 << 12),
	DECLARATION_TYPE_POLYMORPHIQUE = (1 << 13),
	DROITE_ASSIGNATION         = (1 << 14),
	DECLARATION_FUT_VALIDEE    = (1 << 15),
	RI_FUT_GENEREE             = (1 << 16),
	CODE_BINAIRE_FUT_GENERE    = (1 << 17),
	COMPILATRICE               = (1 << 18),
	FORCE_SANSBROYAGE          = (1 << 19),
	EST_RACINE                 = (1 << 20),
};

DEFINIE_OPERATEURS_DRAPEAU(drapeaux_noeud, unsigned int)

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

	EST_NOEUD_ACCES,
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

struct NoeudExpression {
	GenreNoeud genre{};
	GenreValeur genre_valeur{};
	char aide_generation_code = 0;
	REMBOURRE(1);
	drapeaux_noeud drapeaux = drapeaux_noeud::AUCUN;
	Lexeme const *lexeme = nullptr;
	IdentifiantCode *ident = nullptr;
	Type *type = nullptr;

	NoeudBloc *bloc_parent = nullptr;

	NoeudExpression *expression_type = nullptr;

	TransformationType transformation{};

	UniteCompilation *unite = nullptr;
	NoeudCode *noeud_code = nullptr;

	NoeudExpression() = default;

	COPIE_CONSTRUCT(NoeudExpression);

#define EST_NOEUD_GENRE(genre_, ...) \
	inline bool est_##genre_() const \
	{ \
		return dls::outils::est_element(this->genre, __VA_ARGS__); \
	}

	EST_NOEUD_GENRE(appel, GenreNoeud::EXPRESSION_APPEL_FONCTION)
	EST_NOEUD_GENRE(args_variadiques, GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES)
	EST_NOEUD_GENRE(assignation, GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE)
	EST_NOEUD_GENRE(bloc, GenreNoeud::INSTRUCTION_COMPOSEE)
	EST_NOEUD_GENRE(booleen, GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN)
	EST_NOEUD_GENRE(boucle, GenreNoeud::INSTRUCTION_BOUCLE)
	EST_NOEUD_GENRE(caractere, GenreNoeud::EXPRESSION_LITTERALE_CARACTERE)
	EST_NOEUD_GENRE(chaine, GenreNoeud::EXPRESSION_LITTERALE_CHAINE)
	EST_NOEUD_GENRE(comme, GenreNoeud::EXPRESSION_COMME)
	EST_NOEUD_GENRE(comparaison_chainee, GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE)
	EST_NOEUD_GENRE(construction_struct, GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE)
	EST_NOEUD_GENRE(construction_tableau, GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU)
	EST_NOEUD_GENRE(controle_boucle, GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)
	EST_NOEUD_GENRE(coroutine, GenreNoeud::DECLARATION_COROUTINE)
	EST_NOEUD_GENRE(decl_discr, GenreNoeud::INSTRUCTION_DISCR, GenreNoeud::INSTRUCTION_DISCR_ENUM, GenreNoeud::INSTRUCTION_DISCR_UNION)
	EST_NOEUD_GENRE(decl_var, GenreNoeud::DECLARATION_VARIABLE)
	EST_NOEUD_GENRE(deloge, GenreNoeud::EXPRESSION_DELOGE)
	EST_NOEUD_GENRE(discr, GenreNoeud::INSTRUCTION_DISCR, GenreNoeud::INSTRUCTION_DISCR_ENUM, GenreNoeud::INSTRUCTION_DISCR_UNION)
	EST_NOEUD_GENRE(enum, GenreNoeud::DECLARATION_ENUM)
	EST_NOEUD_GENRE(execute, GenreNoeud::DIRECTIVE_EXECUTION)
	EST_NOEUD_GENRE(expansion_variadique, GenreNoeud::EXPANSION_VARIADIQUE)
	EST_NOEUD_GENRE(fonction, GenreNoeud::DECLARATION_FONCTION)
	EST_NOEUD_GENRE(indexage, GenreNoeud::EXPRESSION_INDEXAGE)
	EST_NOEUD_GENRE(info_de, GenreNoeud::EXPRESSION_INFO_DE)
	EST_NOEUD_GENRE(init_de, GenreNoeud::EXPRESSION_INIT_DE)
	EST_NOEUD_GENRE(loge, GenreNoeud::EXPRESSION_LOGE)
	EST_NOEUD_GENRE(memoire, GenreNoeud::EXPRESSION_MEMOIRE)
	EST_NOEUD_GENRE(nombre_entier, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER)
	EST_NOEUD_GENRE(nombre_reel, GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL)
	EST_NOEUD_GENRE(non_initialisation, GenreNoeud::INSTRUCTION_NON_INITIALISATION)
	EST_NOEUD_GENRE(nul, GenreNoeud::EXPRESSION_LITTERALE_NUL)
	EST_NOEUD_GENRE(operateur, GenreNoeud::DECLARATION_OPERATEUR)
	EST_NOEUD_GENRE(operateur_binaire, GenreNoeud::OPERATEUR_BINAIRE)
	EST_NOEUD_GENRE(operateur_unaire, GenreNoeud::OPERATEUR_UNAIRE)
	EST_NOEUD_GENRE(parenthese, GenreNoeud::EXPRESSION_PARENTHESE)
	EST_NOEUD_GENRE(plage, GenreNoeud::EXPRESSION_PLAGE)
	EST_NOEUD_GENRE(pour, GenreNoeud::INSTRUCTION_POUR)
	EST_NOEUD_GENRE(pousse_contexte, GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE)
	EST_NOEUD_GENRE(ref_decl, GenreNoeud::EXPRESSION_REFERENCE_DECLARATION)
	EST_NOEUD_GENRE(ref_membre, GenreNoeud::EXPRESSION_REFERENCE_MEMBRE)
	EST_NOEUD_GENRE(ref_membre_union, GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION)
	EST_NOEUD_GENRE(ref_type, GenreNoeud::EXPRESSION_REFERENCE_TYPE)
	EST_NOEUD_GENRE(reloge, GenreNoeud::EXPRESSION_RELOGE)
	EST_NOEUD_GENRE(repete, GenreNoeud::INSTRUCTION_REPETE)
	EST_NOEUD_GENRE(retiens, GenreNoeud::INSTRUCTION_RETIENS)
	EST_NOEUD_GENRE(retour, GenreNoeud::INSTRUCTION_RETOUR, GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE, GenreNoeud::INSTRUCTION_RETOUR_SIMPLE)
	EST_NOEUD_GENRE(saufsi, GenreNoeud::INSTRUCTION_SAUFSI)
	EST_NOEUD_GENRE(si, GenreNoeud::INSTRUCTION_SI)
	EST_NOEUD_GENRE(sinon, GenreNoeud::INSTRUCTION_SINON)
	EST_NOEUD_GENRE(structure, GenreNoeud::DECLARATION_STRUCTURE)
	EST_NOEUD_GENRE(taille, GenreNoeud::EXPRESSION_TAILLE_DE)
	EST_NOEUD_GENRE(tantque, GenreNoeud::INSTRUCTION_TANTQUE)
	EST_NOEUD_GENRE(tente, GenreNoeud::INSTRUCTION_TENTE)
	EST_NOEUD_GENRE(type_de, GenreNoeud::EXPRESSION_TYPE_DE)

#undef EST_NOEUD_GENRE

#define COMME_NOEUD(genre, type_noeud) \
	inline type_noeud *comme_##genre(); \
	inline type_noeud const *comme_##genre() const;

	COMME_NOEUD(appel, NoeudExpressionAppel)
	COMME_NOEUD(args_variadiques, NoeudTableauArgsVariadiques)
	COMME_NOEUD(assignation, NoeudExpressionBinaire)
	COMME_NOEUD(bloc, NoeudBloc)
	COMME_NOEUD(boucle, NoeudBoucle)
	COMME_NOEUD(comme, NoeudExpressionBinaire)
	COMME_NOEUD(comparaison_chainee, NoeudExpressionBinaire)
	COMME_NOEUD(construction_struct, NoeudExpressionAppel)
	COMME_NOEUD(construction_tableau, NoeudExpressionUnaire)
	COMME_NOEUD(controle_boucle, NoeudExpressionUnaire)
	COMME_NOEUD(coroutine, NoeudDeclarationFonction)
	COMME_NOEUD(decl_discr, NoeudDiscr)
	COMME_NOEUD(decl_var, NoeudDeclarationVariable)
	COMME_NOEUD(deloge, NoeudExpressionLogement)
	COMME_NOEUD(discr, NoeudDiscr)
	COMME_NOEUD(enum, NoeudEnum)
	COMME_NOEUD(execute, NoeudDirectiveExecution)
	COMME_NOEUD(expansion_variadique, NoeudExpressionUnaire)
	COMME_NOEUD(fonction, NoeudDeclarationFonction)
	COMME_NOEUD(indexage, NoeudExpressionBinaire)
	COMME_NOEUD(info_de, NoeudExpressionUnaire)
	COMME_NOEUD(loge, NoeudExpressionLogement)
	COMME_NOEUD(memoire, NoeudExpressionUnaire)
	COMME_NOEUD(operateur, NoeudDeclarationFonction)
	COMME_NOEUD(operateur_binaire, NoeudExpressionBinaire)
	COMME_NOEUD(operateur_unaire, NoeudExpressionUnaire)
	COMME_NOEUD(parenthese, NoeudExpressionUnaire)
	COMME_NOEUD(plage, NoeudExpressionBinaire)
	COMME_NOEUD(pour, NoeudPour)
	COMME_NOEUD(pousse_contexte, NoeudPousseContexte)
	COMME_NOEUD(ref_decl, NoeudExpressionReference)
	COMME_NOEUD(ref_membre, NoeudExpressionMembre)
	COMME_NOEUD(ref_membre_union, NoeudExpressionMembre)
	COMME_NOEUD(reloge, NoeudExpressionLogement)
	COMME_NOEUD(repete, NoeudBoucle)
	COMME_NOEUD(retiens, NoeudExpressionUnaire)
	COMME_NOEUD(retour, NoeudExpressionUnaire)
	COMME_NOEUD(saufsi, NoeudSi)
	COMME_NOEUD(si, NoeudSi)
	COMME_NOEUD(structure, NoeudStruct)
	COMME_NOEUD(taille, NoeudExpressionUnaire)
	COMME_NOEUD(tantque, NoeudBoucle)
	COMME_NOEUD(tente, NoeudTente)
	COMME_NOEUD(type_de, NoeudExpressionUnaire)

#undef COMME_NOEUD
};

struct NoeudDeclaration : public NoeudExpression {
	NoeudDependance *noeud_dependance = nullptr;
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

	NoeudDeclaration *declaration_vient_d_un_emploi = nullptr;
	int index_membre_employe = 0;

	// pour les variables globales
	kuri::tableau<NoeudExpression *> arbre_aplatis{};
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
	NoeudExpression *expr_taille = nullptr;
	NoeudBloc *bloc = nullptr;

	COPIE_CONSTRUCT(NoeudExpressionLogement);
};

struct NoeudDeclarationFonction : public NoeudDeclaration {
	NoeudDeclarationFonction() { genre = GenreNoeud::DECLARATION_FONCTION; }

	kuri::tableau<NoeudDeclaration *> params{};
	kuri::tableau<NoeudExpression *> params_sorties{};
	kuri::tableau<NoeudExpression *> arbre_aplatis_entete{};
	kuri::tableau<NoeudExpression *> arbre_aplatis{};

	kuri::tableau<dls::chaine> noms_retours{};

	NoeudBloc *bloc = nullptr;

	kuri::tableau<dls::vue_chaine_compacte> noms_types_gabarits{};
	bool est_coroutine = false;
	bool est_gabarit = false;
	bool est_variadique = false;
	bool est_externe = false;
	bool est_declaration_type = false;
	bool est_instantiation_gabarit = false;
	dls::chaine nom_broye = "";

	using tableau_paire_expansion = dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>>;

	// À FAIRE : ceci duplique la mémoire du tableau en-bas, peut-êre pourrions nous avoir un tableau référencé, ou un vue_tableau
	tableau_paire_expansion paires_expansion_gabarit{};

	// mise en cache des expansions polymorphiques déjà existantes afin de ne pas les recréer
	// devra être protégé par un mutex quand le typage sera asynchrone
	dls::tableau<std::pair<tableau_paire_expansion, NoeudDeclarationFonction *>> epandu_pour{};

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
	kuri::tableau<NoeudExpression *> arbre_aplatis{};

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

	NoeudExpression *appartiens_a_boucle = nullptr;

	template <typename T>
	using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T>>;

	tableau_synchrone<NoeudDeclaration *> membres{};
	tableau_synchrone<NoeudExpression *>  expressions{};

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

struct NoeudDirectiveExecution : NoeudExpression {
	NoeudDirectiveExecution() { genre = GenreNoeud::DIRECTIVE_EXECUTION; }

	COPIE_CONSTRUCT(NoeudDirectiveExecution);

	NoeudExpression *expr = nullptr;
	NoeudDeclarationFonction *fonction = nullptr;
	AtomeFonction *fonction_ri_pour_appel = nullptr;

	kuri::tableau<NoeudExpression *> arbre_aplatis{};
};

#define COMME_NOEUD(genre, type_noeud) \
	inline type_noeud *NoeudExpression::comme_##genre() \
	{ \
		assert(est_##genre()); \
		return static_cast<type_noeud *>(this); \
	} \
	inline type_noeud const *NoeudExpression::comme_##genre() const \
	{ \
		assert(est_##genre()); \
		return static_cast<type_noeud const *>(this); \
	}

	COMME_NOEUD(appel, NoeudExpressionAppel)
	COMME_NOEUD(args_variadiques, NoeudTableauArgsVariadiques)
	COMME_NOEUD(assignation, NoeudExpressionBinaire)
	COMME_NOEUD(bloc, NoeudBloc)
	COMME_NOEUD(boucle, NoeudBoucle)
	COMME_NOEUD(comme, NoeudExpressionBinaire)
	COMME_NOEUD(comparaison_chainee, NoeudExpressionBinaire)
	COMME_NOEUD(construction_struct, NoeudExpressionAppel)
	COMME_NOEUD(construction_tableau, NoeudExpressionUnaire)
	COMME_NOEUD(controle_boucle, NoeudExpressionUnaire)
	COMME_NOEUD(coroutine, NoeudDeclarationFonction)
	COMME_NOEUD(decl_discr, NoeudDiscr)
	COMME_NOEUD(decl_var, NoeudDeclarationVariable)
	COMME_NOEUD(deloge, NoeudExpressionLogement)
	COMME_NOEUD(discr, NoeudDiscr)
	COMME_NOEUD(enum, NoeudEnum)
	COMME_NOEUD(execute, NoeudDirectiveExecution)
	COMME_NOEUD(expansion_variadique, NoeudExpressionUnaire)
	COMME_NOEUD(fonction, NoeudDeclarationFonction)
	COMME_NOEUD(indexage, NoeudExpressionBinaire)
	COMME_NOEUD(info_de, NoeudExpressionUnaire)
	COMME_NOEUD(loge, NoeudExpressionLogement)
	COMME_NOEUD(memoire, NoeudExpressionUnaire)
	COMME_NOEUD(operateur, NoeudDeclarationFonction)
	COMME_NOEUD(operateur_binaire, NoeudExpressionBinaire)
	COMME_NOEUD(operateur_unaire, NoeudExpressionUnaire)
	COMME_NOEUD(parenthese, NoeudExpressionUnaire)
	COMME_NOEUD(plage, NoeudExpressionBinaire)
	COMME_NOEUD(pour, NoeudPour)
	COMME_NOEUD(pousse_contexte, NoeudPousseContexte)
	COMME_NOEUD(ref_decl, NoeudExpressionReference)
	COMME_NOEUD(ref_membre, NoeudExpressionMembre)
	COMME_NOEUD(ref_membre_union, NoeudExpressionMembre)
	COMME_NOEUD(reloge, NoeudExpressionLogement)
	COMME_NOEUD(repete, NoeudBoucle)
	COMME_NOEUD(retiens, NoeudExpressionUnaire)
	COMME_NOEUD(retour, NoeudExpressionUnaire)
	COMME_NOEUD(saufsi, NoeudSi)
	COMME_NOEUD(si, NoeudSi)
	COMME_NOEUD(structure, NoeudStruct)
	COMME_NOEUD(taille, NoeudExpressionUnaire)
	COMME_NOEUD(tantque, NoeudBoucle)
	COMME_NOEUD(tente, NoeudTente)
	COMME_NOEUD(type_de, NoeudExpressionUnaire)

#undef COMME_NOEUD

void imprime_arbre(NoeudExpression *racine, std::ostream &os, int tab);

bool est_expression_logique(NoeudExpression *noeud);

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
	rassemble_feuilles(expr->expr1, feuilles);
	feuilles.pousse(expr->expr2);
}

NoeudExpression *copie_noeud(
		assembleuse_arbre *assem,
		NoeudExpression const *racine,
		NoeudBloc *bloc_parent);

void aplatis_arbre(
		NoeudExpression *racine,
		kuri::tableau<NoeudExpression *> &arbre_aplatis,
		drapeaux_noeud drapeau);

struct Etendue {
	long pos_min = 0;
	long pos_max = 0;
};

Etendue calcule_etendue_noeud(NoeudExpression *racine, Fichier *fichier);
