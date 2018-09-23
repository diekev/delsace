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

#include <vector>

#include <math/point3.h>
#include <math/vec3.h>

#include "bibliotheques/transformation/transformation.h"

#include "boite_englobante.h"

class Nuanceur;
class Rayon;

/**
 * Représentation d'un triangle et de son vecteur normal dans l'espace
 * tridimensionel.
 */
struct Triangle {
	numero7::math::vec3d v0;
	numero7::math::vec3d v1;
	numero7::math::vec3d v2;

	numero7::math::vec3d normal;
};

/**
 * La classe Maillage contient les triangles formant un objet dans l'espace
 * tridimensionel.
 */
class Maillage {
	std::vector<Triangle *> m_triangles{};

	BoiteEnglobante m_boite_englobante;
	math::transformation m_transformation;

	Nuanceur *m_nuanceur = nullptr;

	std::string m_nom = "maillage";

	bool m_dessine_normaux = false;

public:
	Maillage();

	~Maillage();

	using iterateur = std::vector<Triangle *>::iterator;
	using const_iterateur = std::vector<Triangle *>::const_iterator;

	/**
	 * Retourne un itérateur pointant vers le début de la liste de triangle de
	 * ce maillage.
	 */
	iterateur begin();

	/**
	 * Retourne un itérateur pointant vers la fin de la liste de triangle de ce
	 * maillage.
	 */
	iterateur end();

	/**
	 * Retourne un itérateur constant pointant vers le début de la liste de
	 * triangle de ce maillage.
	 */
	const_iterateur begin() const;

	/**
	 * Retourne un itérateur constant pointant vers la fin de la liste de
	 * triangle de ce maillage.
	 */
	const_iterateur end() const;

	/**
	 * Ajoute un triangle à partir des trois points passés en paramètres. Le
	 * normal du triangle est calculé en fonction de ces trois points.
	 */
	void ajoute_triangle(
			const numero7::math::vec3d &v0,
			const numero7::math::vec3d &v1,
			const numero7::math::vec3d &v2);

	void transformation(const math::transformation &transforme);

	/**
	 * Retourne la transformation de ce maillage.
	 */
	const math::transformation &transformation() const;

	/**
	 * Retourne la boîte englobante de ce maillage.
	 */
	const BoiteEnglobante &boite_englobante() const;

	void nuanceur(Nuanceur *n);

	Nuanceur *nuanceur() const;

	/**
	 * Calcule la boîte englobante de ce maillage.
	 */
	void calcule_boite_englobante();

	/**
	 * Crée un maillage dont les triangles forment un cube d'une unité de rayon.
	 */
	static Maillage *cree_cube();

	/**
	 * Crée un maillage dont les triangles forment une sphère UV d'une unité de
	 * rayon.
	 */
	static Maillage *cree_sphere_uv();

	/**
	 * Crée un maillage dont les triangles forment un plan.
	 */
	static Maillage *cree_plan();

	/**
	 * Calcul les limites de l'objet selon l'algorithme des volumes englobants.
	 */
	void calcule_limites(const numero7::math::vec3d &normal, double &d_proche, double &d_eloigne) const;

	/**
	 * Modifie le nom de ce maillage en fonction de celui passé en paramètre.
	 */
	void nom(const std::string &nom);

	/**
	 * Retourne le nom de ce maillage.
	 */
	const std::string &nom() const;

	/**
	 * Défini si oui ou non il faut dessiner les vecteurs normaux de ce maillage.
	 */
	void dessine_normaux(bool ouinon)
	{
		m_dessine_normaux = ouinon;
	}

	/**
	 * Retourne si oui ou non il faut dessiner les vecteurs normaux de ce maillage.
	 */
	bool dessine_normaux() const
	{
		return m_dessine_normaux;
	}
};

/**
 * Calcul et retourne le normal du triangle spécifié.
 */
numero7::math::vec3d calcul_normal(const Triangle &triangle);

/**
 * Retourne vrai s'il y a entresection entre le triangle et le rayon spécifiés.
 * Si oui, la distance spécifiée est mise à jour.
 */
bool entresecte_triangle(
		const Triangle &triangle,
		const Rayon &rayon,
		double &distance);
