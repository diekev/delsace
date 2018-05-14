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
#include <glm/glm.hpp>
#include <unordered_map>

namespace kangao {

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
};

struct Propriete {
	std::experimental::any valeur;
	TypePropriete type;

	bool visible;
};

/**
 * La classe Manipulable représente un objet qui peut être manipulé dans
 * l'interface. Les propriétés du manipulable sont celles qui seront attachées
 * aux contrôles par l'assembleur. Il est nécessaire que l'instance possède
 * toutes les propriétés utilisées dans le script de définition de l'interface.
 */
class Manipulable {
	std::unordered_map<std::string, Propriete> m_proprietes{};
	bool m_initialise = false;

public:
	Manipulable() = default;

	virtual	~Manipulable() = default;

	using iterateur = std::unordered_map<std::string, Propriete>::iterator;

	/**
	 * Retourne un itérateur pointant vers le début de la liste de propriétés.
	 */
	iterateur debut();

	/**
	 * Retourne un itérateur pointant vers la fin de la liste de propriétés.
	 */
	iterateur fin();

	/**
	 * Retourne si oui ou non le manipulable a été initialisé. Cette propriété
	 * est utilisée pour définir si oui ou non les contrôles créés dans
	 * l'interface doivent prendre leurs valeurs par défaut, ou celles du
	 * manipulable.
	 */
	bool est_initialise() const;

	/**
	 * Marque le manipulable comme étant initialisé.
	 */
	void initialise();

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const std::string &nom, TypePropriete type, const std::experimental::any &valeur);

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const std::string &nom, TypePropriete type);

	/**
	 * Évalue la valeur d'une propriété de type 'entier' du nom spécifié.
	 */
	int evalue_entier(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'décimal' du nom spécifié.
	 */
	float evalue_decimal(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'vecteur' du nom spécifié.
	 */
	glm::vec3 evalue_vecteur(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'couleur' du nom spécifié.
	 */
	glm::vec3 evalue_couleur(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'fichier_entrée' du nom spécifié.
	 */
	std::string evalue_fichier_entree(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'fichier_sortie' du nom spécifié.
	 */
	std::string evalue_fichier_sortie(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'chaine' du nom spécifié.
	 */
	std::string evalue_chaine(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'bool' du nom spécifié.
	 */
	bool evalue_bool(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'liste' du nom spécifié.
	 */
	std::string evalue_liste(const std::string &nom);

	/**
	 * Rends la propriété spécifiée visible dans l'interface.
	 */
	void rend_propriete_visible(const std::string &nom, bool ouinon);

	/**
	 * Ajourne les propriétés de ce manipulable. Par exemple pour décider si
	 * une ou l'autre propriété doit être visible ou non.
	 */
	virtual bool ajourne_proprietes();

	/**
	 * Établie la valeur de la propriété de type bool spécifiée.
	 */
	void valeur_bool(const std::string &nom, bool valeur);

	/**
	 * Établie la valeur de la propriété de type entier spécifiée.
	 */
	void valeur_entier(const std::string &nom, int valeur);

	/**
	 * Établie la valeur de la propriété de type décimal spécifiée.
	 */
	void valeur_decimal(const std::string &nom, float valeur);

	/**
	 * Établie la valeur de la propriété de type vecteur spécifiée.
	 */
	void valeur_vecteur(const std::string &nom, const glm::vec3 &valeur);

	/**
	 * Établie la valeur de la propriété de type couleur spécifiée.
	 */
	void valeur_couleur(const std::string &nom, const glm::vec3 &valeur);

	/**
	 * Établie la valeur de la propriété de type chaine, fichier, ou liste
	 * spécifiée.
	 */
	void valeur_chaine(const std::string &nom, const std::string &valeur);

	/**
	 * Retourne un pointeur vers la valeur de la propriété au nom spécifié.
	 */
	void *operator[](const std::string &nom);
};

}  /* namespace kangao */
