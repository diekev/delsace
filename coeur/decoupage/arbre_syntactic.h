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
#include <list>

#include "donnees_type.h"
#include "morceaux.h"

char caractere_echape(char const *sequence);

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

struct ContexteGenerationCode;

enum class type_noeud : char {
	RACINE,
	DECLARATION_FONCTION,
	APPEL_FONCTION,
	VARIABLE,
	ACCES_MEMBRE,
	ACCES_MEMBRE_POINT,
	CONSTANTE,
	DECLARATION_VARIABLE,
	ASSIGNATION_VARIABLE,
	NOMBRE_REEL,
	NOMBRE_ENTIER,
	OPERATION_BINAIRE,
	OPERATION_UNAIRE,
	RETOUR,
	CHAINE_LITTERALE,
	BOOLEEN,
	CARACTERE,
	SI,
	BLOC,
	POUR,
	CONTINUE_ARRETE,
	BOUCLE,
	TANTQUE,
	TRANSTYPE,
	MEMOIRE,
	NUL,
	TAILLE_DE,
	PLAGE,
	DIFFERE,
	NONSUR,
	TABLEAU,
	CONSTRUIT_TABLEAU,
	CONSTRUIT_STRUCTURE,
	TYPE_DE,
	LOGE,
	DELOGE,
	RELOGE,
};

const char *chaine_type_noeud(type_noeud type);

/* ************************************************************************** */

/* Notes pour supprimer le std::list de la structure noeud et n'utiliser de la
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

enum : unsigned short {
	DYNAMIC           = (1 << 0),
	VARIADIC          = (1 << 1),
	GLOBAL            = (1 << 2),
	CONVERTI_TABLEAU  = (1 << 3),
	CONVERTI_EINI     = (1 << 4),
	EXTRAIT_EINI      = (1 << 5),
	EXTRAIT_CHAINE_C  = (1 << 6),
	INDIRECTION_APPEL = (1 << 7),
	EST_EXTERNE       = (1 << 8),
	EST_CALCULE       = (1 << 9),

	MASQUE_CONVERSION = CONVERTI_EINI | CONVERTI_TABLEAU | EXTRAIT_EINI | EXTRAIT_CHAINE_C,
};

inline bool possede_drapeau(unsigned short drapeau, unsigned short valeur)
{
	return (drapeau & valeur) != 0;
}

namespace noeud {

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
struct base {
	std::list<base *> enfants{};
	DonneesMorceaux const &morceau;

	std::any valeur_calculee{};

	size_t index_type = -1ul;

	bool pad = false;
	unsigned short drapeaux = 0;
	type_noeud type{};
	int module_appel{}; // module pour les appels de fonctions importées

	explicit base(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

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
	id_morceau identifiant() const;

	/**
	 * Retourne une référence constante vers la chaine du morceau de ce noeud.
	 */
	std::string_view const &chaine() const;

	/**
	 * Retourne une référence constante vers les données du morceau de ce neoud.
	 */
	DonneesMorceaux const &donnees_morceau() const;

	/**
	 * Retourne un pointeur vers le dernier enfant de ce noeud. Si le noeud n'a
	 * aucun enfant, retourne nullptr.
	 */
	base *dernier_enfant() const;
};

void rassemble_feuilles(
		base *noeud_base,
		std::vector<base *> &feuilles);

bool est_constant(base *b);

llvm::Value *genere_code_llvm(base *b, ContexteGenerationCode &contexte, bool expr_gauche);

void performe_validation_semantique(base *b, ContexteGenerationCode &contexte);

/* Ajout le nom d'un argument à la liste des noms d'un noeud d'appel */
void ajoute_nom_argument(base *b, const std::string_view &nom);

void verifie_compatibilite(
		base *b,
		ContexteGenerationCode &contexte,
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		base *enfant);

bool peut_operer(
		const DonneesType &type1,
		const DonneesType &type2,
		type_noeud type_gauche,
		type_noeud type_droite);

}  /* namespace noeud */
