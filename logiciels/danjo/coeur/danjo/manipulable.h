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

#include <experimental/any>

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/couleur.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

struct CourbeBezier;
struct CourbeCouleur;
struct RampeCouleur;

namespace danjo {

enum TypePropriete {
	ENTIER,
	DECIMAL,
	VECTEUR,
	COULEUR,
	FICHIER_ENTREE,
	FICHIER_SORTIE,
	CHAINE_CARACTERE,
	BOOL,
	ENUM,
	COURBE_COULEUR,
	COURBE_VALEUR,
	RAMPE_COULEUR,
	TEXTE,
};

struct Propriete {
	std::experimental::any valeur{};
	TypePropriete type{};

	bool est_extra = false;

	bool visible = true;

	bool pad[2];

	dls::tableau<std::pair<int, std::experimental::any>> courbe{};

	void ajoute_cle(const int v, int temps);

	void ajoute_cle(const float v, int temps);

	void ajoute_cle(const dls::math::vec3f &v, int temps);

	void ajoute_cle(const dls::phys::couleur32 &v, int temps);

	void supprime_animation();

	bool est_anime() const;

	bool possede_cle(int temps) const;

	int evalue_entier(int temps) const;

	float evalue_decimal(int temps) const;

	dls::math::vec3f evalue_vecteur(int temps) const;

	dls::phys::couleur32 evalue_couleur(int temps) const;

private:
	void ajoute_cle_impl(const std::experimental::any &v, int temps);

	void tri_courbe();

	bool trouve_valeurs_temps(int temps, std::experimental::any &v1, std::experimental::any &v2, int &t1, int &t2) const;
};

/**
 * La classe Manipulable représente un objet qui peut être manipulé dans
 * l'entreface. Les propriétés du manipulable sont celles qui seront attachées
 * aux contrôles par l'assembleur. Il est nécessaire que l'instance possède
 * toutes les propriétés utilisées dans le script de définition de l'entreface.
 */
class Manipulable {
	dls::dico_desordonne<dls::chaine, Propriete> m_proprietes{};
	bool m_initialise = false;
	bool pad[7];

public:
	Manipulable() = default;

	virtual	~Manipulable() = default;

	using iterateur = dls::dico_desordonne<dls::chaine, Propriete>::iteratrice;

	/**
	 * Retourne un itérateur pointant vers le début de la liste de propriétés.
	 */
	iterateur debut();

	/**
	 * Retourne un itérateur pointant vers la fin de la liste de propriétés.
	 */
	iterateur fin();

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const dls::chaine &nom, TypePropriete type, const std::experimental::any &valeur);

	/**
	 * Ajoute une propriété extra à ce manipulable avec le nom spécifié.
	 */
	void ajoute_propriete_extra(const dls::chaine &nom, const Propriete &propriete);

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const dls::chaine &nom, TypePropriete type);

	/**
	 * Évalue la valeur d'une propriété de type 'entier' du nom spécifié.
	 */
	int evalue_entier(const dls::chaine &nom, int temps = 0) const;

	/**
	 * Évalue la valeur d'une propriété de type 'décimal' du nom spécifié.
	 */
	float evalue_decimal(const dls::chaine &nom, int temps = 0) const;

	/**
	 * Évalue la valeur d'une propriété de type 'vecteur' du nom spécifié.
	 */
	dls::math::vec3f evalue_vecteur(const dls::chaine &nom, int temps = 0) const;

	/**
	 * Évalue la valeur d'une propriété de type 'couleur' du nom spécifié.
	 */
	dls::phys::couleur32 evalue_couleur(const dls::chaine &nom, int temps = 0) const;

	/**
	 * Évalue la valeur d'une propriété de type 'fichier_entrée' du nom spécifié.
	 */
	dls::chaine evalue_fichier_entree(const dls::chaine &nom) const;

	/**
	 * Évalue la valeur d'une propriété de type 'fichier_sortie' du nom spécifié.
	 */
	dls::chaine evalue_fichier_sortie(const dls::chaine &nom) const;

	/**
	 * Évalue la valeur d'une propriété de type 'chaine' du nom spécifié.
	 */
	dls::chaine evalue_chaine(const dls::chaine &nom) const;

	/**
	 * Évalue la valeur d'une propriété de type 'bool' du nom spécifié.
	 */
	bool evalue_bool(const dls::chaine &nom) const;

	/**
	 * Évalue la valeur d'une propriété de type 'énum' du nom spécifié.
	 */
	dls::chaine evalue_enum(const dls::chaine &nom) const;

	/**
	 * Évalue la valeur d'une propriété de type 'liste' du nom spécifié.
	 */
	dls::chaine evalue_liste(const dls::chaine &nom) const;

	/**
	 * Retourne la courbe de la propriété 'courbe_couleur' du nom spécifié.
	 */
	CourbeCouleur const *evalue_courbe_couleur(const dls::chaine &nom) const;

	/**
	 * Retourne la courbe de la propriété 'courbe_valeur' du nom spécifié.
	 */
	CourbeBezier const *evalue_courbe_valeur(const dls::chaine &nom) const;

	/**
	 * Retourne la rampe de la propriété 'rampe_couleur' du nom spécifié.
	 */
	RampeCouleur const *evalue_rampe_couleur(const dls::chaine &nom) const;

	/**
	 * Rends la propriété spécifiée visible dans l'entreface.
	 */
	void rend_propriete_visible(const dls::chaine &nom, bool ouinon);

	/**
	 * Ajourne les propriétés de ce manipulable. Par exemple pour décider si
	 * une ou l'autre propriété doit être visible ou non.
	 */
	virtual bool ajourne_proprietes();

	/**
	 * Établie la valeur de la propriété de type bool spécifiée.
	 */
	void valeur_bool(const dls::chaine &nom, bool valeur);

	/**
	 * Établie la valeur de la propriété de type entier spécifiée.
	 */
	void valeur_entier(const dls::chaine &nom, int valeur);

	/**
	 * Établie la valeur de la propriété de type décimal spécifiée.
	 */
	void valeur_decimal(const dls::chaine &nom, float valeur);

	/**
	 * Établie la valeur de la propriété de type vecteur spécifiée.
	 */
	void valeur_vecteur(const dls::chaine &nom, const dls::math::vec3f &valeur);

	/**
	 * Établie la valeur de la propriété de type couleur spécifiée.
	 */
	void valeur_couleur(const dls::chaine &nom, const dls::phys::couleur32 &valeur);

	/**
	 * Établie la valeur de la propriété de type chaine, fichier, ou liste
	 * spécifiée.
	 */
	void valeur_chaine(const dls::chaine &nom, const dls::chaine &valeur);

	/**
	 * Retourne un pointeur vers la valeur de la propriété au nom spécifié.
	 */
	void *operator[](const dls::chaine &nom);

	/**
	 * Retourne le type de la propriété du nom spécifié.
	 */
	TypePropriete type_propriete(const dls::chaine &nom) const;

	Propriete *propriete(const dls::chaine &nom);

	Propriete const *propriete(const dls::chaine &nom) const;
};

}  /* namespace danjo */
