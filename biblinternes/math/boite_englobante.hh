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

#include "vecteur.hh"

#include "../outils/constantes.h"

/**
 * La classe BoiteEnglobante représente le volume englobant un objet dans
 * l'espace tridimensionnel.
 */
class BoiteEnglobante {
public:
	dls::math::point3d min = dls::math::point3d( constantes<double>::INFINITE);
	dls::math::point3d max = dls::math::point3d(-constantes<double>::INFINITE);

	/**
	 * Construit une instance de BoiteEnglobante avec des valeurs par défaut.
	 * Les limites de la boîte sont définies pour être dégénérées : min > max.
	 */
	BoiteEnglobante() = default;

	/**
	 * Construit une instance de BoiteEnglobante à partir d'un simple point.
	 * Après cette opération, l'expression min == max == point est vraie.
	 */
	explicit BoiteEnglobante(dls::math::point3d const &point);

	/**
	 * Construit une instance de BoiteEnglobante à partir de deux points. Les
	 * coordonnées minimum et maximum de la boîte sont définies comme étant les
	 * valeurs minimum et maximum des coordonnées des points d'entrées ; ainsi
	 * les points spécifiés n'ont pas besoin d'être ordonnés.
	 */
	BoiteEnglobante(
			dls::math::point3d const &p1,
			dls::math::point3d const &p2);

	/**
	 * Retourne vrai s'il y a chevauchement entre la boîte spécifiée et celle-ci.
	 */
	bool chevauchement(BoiteEnglobante const &boite);

	/**
	 * Retourne vrai si le point spécifié est contenu dans cette boîte.
	 */
	bool contient(dls::math::point3d const &point);

	/**
	 * Étend les limite de la boîte selon le delta spécifié, qui est enlevé au
	 * point minimum, et ajouté au point maximum.
	 */
	void etend(double delta);

	/**
	 * Retourne l'aire de la surface des six faces de cette boîte.
	 */
	double aire_surface() const;

	/**
	 * Retourne le volume de cette boîte.
	 */
	double volume() const;

	/**
	 * Retourne l'index (0, 1, 2) de la dimension (x, y, z) ayant la plus grande
	 * ampleur.
	 */
	int ampleur_maximale() const;

	/**
	 * Retourne la position d'un point relativement aux coordonnées de cette
	 * boîte. Un point se situant au coin minimum de cette boîte aura un
	 * décalage de (0, 0, 0), alors qu'un point au coin maximum en aura un de
	 * (1, 1, 1), etc.
	 */
	dls::math::vec3d decalage(dls::math::point3d const &point);
};

/**
 * Performe l'union d'une boîte et d'un point et retourne la boîte y résultant.
 */
BoiteEnglobante unie(
		BoiteEnglobante const &boite,
		dls::math::point3d const &point);

/**
 * Performe l'union de deux boîtes et retourne la boîte y résultant.
 */
BoiteEnglobante unie(
		BoiteEnglobante const &boite1,
		BoiteEnglobante const &boite2);
