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

#include <delsace/math/matrice.hh>
#include <delsace/math/vecteur.hh>

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
	dls::math::mat4x4f m_modele_vue{};
	dls::math::mat4x4f m_projection{};
	dls::math::mat4x4f m_modele_vue_projection{};
	dls::math::vec3f m_vue{};
	dls::math::mat3x3f m_normal{};
	dls::math::mat4x4f m_matrice_objet{};
	bool m_pour_surlignage = false;
	bool m_dessine_arretes = false;
	bool m_dessine_normaux = false;

public:
	/**
	 * Retourne la matrice de modèle-vue.
	 */
	const dls::math::mat4x4f &modele_vue() const;

	/**
	 * Change la matrice de modèle-vue.
	 */
	void modele_vue(const dls::math::mat4x4f &matrice);

	/**
	 * Retourne la matrice de projection.
	 */
	const dls::math::mat4x4f &projection() const;

	/**
	 * Change la matrice de projection.
	 */
	void projection(const dls::math::mat4x4f &matrice);

	/**
	 * Retourne la direction vers laquelle la caméra pointe.
	 */
	const dls::math::vec3f &vue() const;

	/**
	 * Change la direction vers laquelle la caméra pointe.
	 */
	void vue(const dls::math::vec3f &matrice);

	/**
	 * Retourne la matrice de normal.
	 */
	const dls::math::mat3x3f &normal() const;

	/**
	 * Change la matrice de normal.
	 */
	void normal(const dls::math::mat3x3f &matrice);

	/**
	 * Retourne la matrice de modèle-vue-projection.
	 */
	const dls::math::mat4x4f &MVP() const;

	/**
	 * Change la matrice de modèle-vue-projection.
	 */
	void MVP(const dls::math::mat4x4f &matrice);

	/**
	 * Retourne la matrice composée de l'objet courant.
	 */
	const dls::math::mat4x4f &matrice_objet() const;

	/**
	 * Change la matrice composée de l'objet courant.
	 */
	void matrice_objet(const dls::math::mat4x4f &matrice);

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
