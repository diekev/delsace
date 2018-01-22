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

#include <unordered_map>
#include <experimental/any>

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

/**
 * La classe Manipulable représente un objet qui peut être manipulé dans
 * l'interface. Les propriétés du manipulable sont celles qui seront attachées
 * aux contrôles par l'assembleur. Il est nécessaire que l'instance possède
 * toutes les propriétés utilisées dans le script de définition de l'interface.
 */
class Manipulable {
	struct Propriete {
		std::experimental::any valeur;
		TypePropriete type;
	};

	std::unordered_map<std::string, Propriete> m_proprietes{};

public:
	Manipulable() = default;

	virtual	~Manipulable() = default;

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
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
	int evalue_vecteur(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'couleur' du nom spécifié.
	 */
	int evalue_couleur(const std::string &nom);

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
	int evalue_liste(const std::string &nom);

	/**
	 * Retourne un pointeur vers la valeur de la propriété au nom spécifié.
	 */
	void *operator[](const std::string &nom);
};
