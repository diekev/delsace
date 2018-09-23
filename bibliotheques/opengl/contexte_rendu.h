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

#include <glm/glm.hpp>

/**
 * La classe ContexteRendu contient les propriétés du contexte OpenGL dans
 * lequel une scène 3D est rendu. Cette classe ne fait que contenir les données,
 * elle ne modifie en rien la scène ou la caméra.
 *
 * Son but premier est de contenir les données principales pour rendre les
 * objets (matrices de la caméra, matrice composée de l'objet, etc...) pour
 * éviter de passer trop d'objets différents en paramètre des fonctions de rendu
 * des tampons entre autres.
 *
 * Cette classe est exclusivement utilisée par VisionneurScene qui la passe aux
 * différentes classes de rendu d'objet dans la scène 3D.
 */
class ContexteRendu {
	glm::mat4 m_modele_vue;
	glm::mat4 m_projection;
	glm::mat4 m_modele_vue_projection;
	glm::vec3 m_vue;
	glm::mat3 m_normal;
	glm::mat4 m_matrice_objet;
	bool m_pour_surlignage = false;
	bool m_dessine_arretes = false;
	bool m_dessine_normaux = false;

public:
	/**
	 * Retourne la matrice de modèle-vue.
	 */
	const glm::mat4 &modele_vue() const;

	/**
	 * Change la matrice de modèle-vue.
	 */
	void modele_vue(const glm::mat4 &matrice);

	/**
	 * Retourne la matrice de projection.
	 */
	const glm::mat4 &projection() const;

	/**
	 * Change la matrice de projection.
	 */
	void projection(const glm::mat4 &matrice);

	/**
	 * Retourne la direction vers laquelle la caméra pointe.
	 */
	const glm::vec3 &vue() const;

	/**
	 * Change la direction vers laquelle la caméra pointe.
	 */
	void vue(const glm::vec3 &matrice);

	/**
	 * Retourne la matrice de normal.
	 */
	const glm::mat3 &normal() const;

	/**
	 * Change la matrice de normal.
	 */
	void normal(const glm::mat3 &matrice);

	/**
	 * Retourne la matrice de modèle-vue-projection.
	 */
	const glm::mat4 &MVP() const;

	/**
	 * Change la matrice de modèle-vue-projection.
	 */
	void MVP(const glm::mat4 &matrice);

	/**
	 * Retourne la matrice composée de l'objet courant.
	 */
	const glm::mat4 &matrice_objet() const;

	/**
	 * Change la matrice composée de l'objet courant.
	 */
	void matrice_objet(const glm::mat4 &matrice);

	/**
	 * Retourne si oui ou non les données sont à utiliser pour surligner, ou
	 * dessiner le contour, de l'objet courant.
	 */
	bool pour_surlignage() const;

	/**
	 * Décide si oui ou non les données sont à utiliser pour surligner, ou
	 * dessiner le contour, de l'objet courant.
	 */
	void pour_surlignage(bool ouinon);

	/**
	 * Retourne si oui ou non les données sont à utiliser pour dessiner les
	 * arrêtes des objets.
	 */
	bool dessine_arretes() const;

	/**
	 * Décide si oui ou non les données sont à utiliser pour dessiner les
	 * arrêtes des objets.
	 */
	void dessine_arretes(bool ouinon);

	/**
	 * Retourne si oui ou non les données sont à utiliser pour dessiner les
	 * normaux des objets.
	 */
	bool dessine_normaux() const;

	/**
	 * Décide si oui ou non les données sont à utiliser pour dessiner les
	 * normaux des objets.
	 */
	void dessine_normaux(bool ouinon);
};
