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
#include <vector>

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
};

struct Propriete {
	std::experimental::any valeur;
	TypePropriete type;

	bool est_extra = false;

	bool visible = true;

	std::vector<std::pair<int, std::experimental::any>> courbe;

	void ajoute_cle(const int v, int temps);

	void ajoute_cle(const float v, int temps);

	void ajoute_cle(const glm::vec3 &v, int temps);

	void ajoute_cle(const glm::vec4 &v, int temps);

	void supprime_animation();

	bool est_anime() const;

	bool possede_cle(int temps) const;

	int evalue_entier(int temps);

	float evalue_decimal(int temps);

	glm::vec3 evalue_vecteur(int temps);

	glm::vec4 evalue_couleur(int temps);

private:
	void ajoute_cle_impl(const std::experimental::any &v, int temps);

	void tri_courbe();

	bool trouve_valeurs_temps(int temps, std::experimental::any &v1, std::experimental::any &v2, int &t1, int &t2);
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
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const std::string &nom, TypePropriete type, const std::experimental::any &valeur);

	/**
	 * Ajoute une propriété extra à ce manipulable avec le nom spécifié.
	 */
	void ajoute_propriete_extra(const std::string &nom, const Propriete &propriete);

	/**
	 * Ajoute une propriété à ce manipulable avec le nom et type spécifiés.
	 *
	 * La valeur spécifiée est la valeur par défaut du manipulable.
	 */
	void ajoute_propriete(const std::string &nom, TypePropriete type);

	/**
	 * Évalue la valeur d'une propriété de type 'entier' du nom spécifié.
	 */
	int evalue_entier(const std::string &nom, int temps = 0);

	/**
	 * Évalue la valeur d'une propriété de type 'décimal' du nom spécifié.
	 */
	float evalue_decimal(const std::string &nom, int temps = 0);

	/**
	 * Évalue la valeur d'une propriété de type 'vecteur' du nom spécifié.
	 */
	glm::vec3 evalue_vecteur(const std::string &nom, int temps = 0);

	/**
	 * Évalue la valeur d'une propriété de type 'couleur' du nom spécifié.
	 */
	glm::vec4 evalue_couleur(const std::string &nom, int temps = 0);

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
	 * Évalue la valeur d'une propriété de type 'énum' du nom spécifié.
	 */
	std::string evalue_enum(const std::string &nom);

	/**
	 * Évalue la valeur d'une propriété de type 'liste' du nom spécifié.
	 */
	std::string evalue_liste(const std::string &nom);

	/**
	 * Retourne la courbe de la propriété 'courbe_couleur' du nom spécifié.
	 */
	CourbeCouleur *evalue_courbe_couleur(const std::string &nom);

	/**
	 * Retourne la courbe de la propriété 'courbe_valeur' du nom spécifié.
	 */
	CourbeBezier *evalue_courbe_valeur(const std::string &nom);

	/**
	 * Retourne la rampe de la propriété 'rampe_couleur' du nom spécifié.
	 */
	RampeCouleur *evalue_rampe_couleur(const std::string &nom);

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
	void valeur_couleur(const std::string &nom, const glm::vec4 &valeur);

	/**
	 * Établie la valeur de la propriété de type chaine, fichier, ou liste
	 * spécifiée.
	 */
	void valeur_chaine(const std::string &nom, const std::string &valeur);

	/**
	 * Retourne un pointeur vers la valeur de la propriété au nom spécifié.
	 */
	void *operator[](const std::string &nom);

	/**
	 * Retourne le type de la propriété du nom spécifié.
	 */
	TypePropriete type_propriete(const std::string &nom);

	Propriete *propriete(const std::string &nom);
};

}  /* namespace danjo */
