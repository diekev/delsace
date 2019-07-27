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
#include "biblinternes/structures/liste.hh"

#include "donnees_type.hh"

struct ContexteGenerationCode;

enum class type_noeud : char {
	RACINE,
	DECLARATION_FONCTION,
	APPEL_FONCTION,
	VARIABLE,
	ACCES_MEMBRE,
	ACCES_MEMBRE_POINT,
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
	DECLARATION_STRUCTURE,
	DECLARATION_ENUM,
	ASSOCIE,
	PAIRE_ASSOCIATION,
	SAUFSI,
	RETIENS,
};

const char *chaine_type_noeud(type_noeud type);

/* ************************************************************************** */

inline bool possede_drapeau(unsigned short drapeau, unsigned short valeur)
{
	return (drapeau & valeur) != 0;
}

enum : unsigned short {
	IGNORE_OPERATEUR = (1 << 0),
};

struct DonneesFonction;

namespace noeud {

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
struct base {
	dls::liste<base *> enfants{};
	DonneesMorceaux const &morceau;

	std::any valeur_calculee{};

	dls::chaine nom_fonction_appel{}; // À FAIRE : on ne peut pas utiliser valeur_calculee car les prépasses peuvent le changer.

	long index_type = -1l;

	char aide_generation_code = 0;
	unsigned short drapeaux = 0;
	type_noeud type{};
	int module_appel{}; // module pour les appels de fonctions importées

	DonneesFonction *df = nullptr; // pour les appels de coroutines dans les boucles ou autres.

	explicit base(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

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
	id_morceau identifiant() const;

	/**
	 * Retourne une référence constante vers la chaine du morceau de ce noeud.
	 */
	dls::vue_chaine const &chaine() const;

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

}  /* namespace noeud */
