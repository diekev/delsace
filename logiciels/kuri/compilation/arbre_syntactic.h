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
#include "biblinternes/structures/liste.hh"

#include "donnees_type.h"
#include "transformation_type.hh"

struct ContexteGenerationCode;

enum class GenreNoeud : char {
	RACINE,
	DECLARATION_COROUTINE,
	DECLARATION_ENUM,
	DECLARATION_FONCTION,
	DECLARATION_PARAMETRES_FONCTION,
	DECLARATION_STRUCTURE,
	DECLARATION_VARIABLE,
	DIRECTIVE_EXECUTION,
	EXPRESSION_APPEL_FONCTION,
	EXPRESSION_ASSIGNATION_VARIABLE,
	EXPRESSION_CONSTRUCTION_STRUCTURE,
	EXPRESSION_CONSTRUCTION_TABLEAU,
	EXPRESSION_DELOGE,
	EXPRESSION_INDICE,
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
	EXPRESSION_TRANSTYPE,
	INSTRUCTION_BOUCLE,
	INSTRUCTION_COMPOSEE,
	INSTRUCTION_CONTINUE_ARRETE,
	INSTRUCTION_DIFFERE,
	INSTRUCTION_DISCR,
	INSTRUCTION_DISCR_ENUM,
	INSTRUCTION_DISCR_UNION,
	INSTRUCTION_POUR,
	INSTRUCTION_NONSUR,
	INSTRUCTION_PAIRE_DISCR,
	INSTRUCTION_REPETE,
	INSTRUCTION_RETIENS,
	INSTRUCTION_RETOUR,
	INSTRUCTION_RETOUR_MULTIPLE,
	INSTRUCTION_RETOUR_SIMPLE,
	INSTRUCTION_SAUFSI,
	INSTRUCTION_SI,
	INSTRUCTION_SINON,
	INSTRUCTION_TANTQUE,
	OPERATEUR_BINAIRE,
	OPERATEUR_COMPARAISON_CHAINEE,
	OPERATEUR_UNAIRE,
};

const char *chaine_genre_noeud(GenreNoeud type);

inline bool est_instruction_retour(GenreNoeud type)
{
	return type == GenreNoeud::INSTRUCTION_RETOUR || type == GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE || type == GenreNoeud::INSTRUCTION_RETOUR_SIMPLE;
}

/* ************************************************************************** */

/* Notes pour supprimer le dls::liste de la structure noeud et n'utiliser de la
 * mémoire que quand nécessaire.
 *
 * noeud racine : multiples enfants pouvant être dans des tableaux différents
 * -- noeud déclaration fonction
 * -- noeud déclaration structure
 * -- noeud déclaration énum
 *
 * noeud déclaration fonction : un seul enfant
 * -- noeud bloc
 *
 * noeud appel fonction : multiples enfants de mêmes types
 * -- noeud expression
 *
 * noeud assignation variable : deux enfants
 * -- noeud expression | noeud déclaration variable
 * -- noeud expression
 *
 * noeud retour : un seul enfant
 * -- noeud expression
 *
 * noeud opérateur binaire : 2 enfants de même type
 * -- noeud expression
 * -- noeud expression
 *
 * noeud opérateur binaire : un seul enfant
 * -- noeud expression
 *
 * noeud expression : un seul enfant, peut utiliser une énumeration pour choisir
 *                    le bon noeud
 * -- noeud (variable | opérateur | nombre entier | nombre réel | appel fonction)
 *
 * noeud accès membre : deux enfants de même type
 * -- noeud variable
 *
 * noeud boucle : un seul enfant
 * -- noeud bloc
 *
 * noeud pour : 4 enfants
 * -- noeud variable
 * -- noeud expression
 * -- noeud expression
 * -- noeud bloc
 *
 * noeud bloc : multiples enfants de types différents
 * -- déclaration variable / expression / retour / boucle pour
 *
 * noeud si : 2 ou 3 enfants
 * -- noeud expression
 * -- noeud bloc
 * -- noeud si | noeud bloc
 *
 * noeud déclaration variable : aucun enfant
 * noeud variable : aucun enfant
 * noeud nombre entier : aucun enfant
 * noeud nombre réel : aucun enfant
 * noeud booléen : aucun enfant
 * noeud chaine caractère : aucun enfant
 * noeud continue_arrête : aucun enfant
 * noeud pointeur nul : aucun enfant
 *
 * Le seul type de neoud posant problème est le noeud de déclaration de
 * fonction, mais nous pourrions avoir des tableaux séparés avec une structure
 * de données pour définir l'ordre d'apparition des noeuds des tableaux dans la
 * fonction. Tous les autres types de noeuds ont des enfants bien défini, donc
 * nous pourrions peut-être supprimer l'héritage, tout en forçant une interface
 * commune à tous les noeuds.
 */

/* ************************************************************************** */

enum drapeaux_noeud : unsigned short {
	AUCUN                  = 0,
	DYNAMIC                = (1 << 0),
	EMPLOYE                = (1 << 1),
	DECLARATION            = (1 << 2),
	EST_EXTERNE            = (1 << 3),
	EST_CALCULE            = (1 << 4),
	IGNORE_OPERATEUR       = (1 << 5),
	FORCE_ENLIGNE          = (1 << 6),
	FORCE_HORSLIGNE        = (1 << 7),
	FORCE_NULCTX           = (1 << 8),
	EST_ASSIGNATION_OPEREE = (1 << 9),
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

struct DonneesFonction;
struct DonneesOperateur;

namespace noeud {

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
struct base {
	dls::liste<base *> enfants{};
	DonneesLexeme const &morceau;

	std::any valeur_calculee{};

	dls::chaine nom_fonction_appel{}; // À FAIRE : on ne peut pas utiliser valeur_calculee car les prépasses peuvent le changer.

	long index_type = -1l;

	/* utilisé pour déterminer les types de retour des fonctions à moultretour
	 * car lors du besoin index_type est utilisé pour le type de retour de la
	 *  première valeur */
	long index_type_fonc = -1l;

	char aide_generation_code = 0;
	GenreNoeud genre{};
	drapeaux_noeud drapeaux = drapeaux_noeud::AUCUN;
	int module_appel{}; // module pour les appels de fonctions importées

	DonneesFonction *df = nullptr; // pour les appels de coroutines dans les boucles ou autres.
	DonneesOperateur const *op = nullptr;

	DonneesTypeDeclare type_declare{};

	GenreValeur genre_valeur = GenreValeur::INVALIDE;

	TransformationType transformation{};

	explicit base(DonneesLexeme const &morceau);

	base(base const &) = default;
	base &operator=(base const &) = default;

	/**
	 * Ajoute un noeud à la liste des noeuds du noeud.
	 */
	void ajoute_noeud(base *noeud);

	/**
	 * Imprime le 'code' de ce noeud dans le flux de sortie 'os' précisé. C'est
	 * attendu que le noeud demande à ces enfants d'imprimer leurs 'codes' dans
	 * le bon ordre.
	 */
	void imprime_code(std::ostream &os, int tab);

	/**
	 * Retourne l'identifiant du morceau de ce noeud.
	 */
	TypeLexeme identifiant() const;

	/**
	 * Retourne une référence constante vers la chaine du morceau de ce noeud.
	 */
	dls::vue_chaine_compacte const &chaine() const;

	/**
	 * Retourne une référence constante vers les données du morceau de ce neoud.
	 */
	DonneesLexeme const &donnees_morceau() const;

	/**
	 * Retourne un pointeur vers le dernier enfant de ce noeud. Si le noeud n'a
	 * aucun enfant, retourne nullptr.
	 */
	base *dernier_enfant() const;

	/* retourne la valeur_calculee avec le type dls::chaine */
	dls::chaine chaine_calculee() const;
};

void rassemble_feuilles(
		base *noeud_base,
		dls::tableau<base *> &feuilles);

/* Ajout le nom d'un argument à la liste des noms d'un noeud d'appel */
void ajoute_nom_argument(base *b, const dls::vue_chaine_compacte &nom);

}  /* namespace noeud */
