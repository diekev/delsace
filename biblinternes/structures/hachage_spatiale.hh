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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <unordered_map>
#include <vector>

#include "biblinternes/math/vecteur.hh"

struct HachageSpatial {
	std::unordered_map<std::size_t, std::vector<dls::math::vec3f>> m_tableau{};

	/**
	 * La taille maximum recommandée par la publication de Cline et al. est de
	 * 20 000. Cependant, les fonctions de hachage marche mieux quand la taille
	 * est un nombre premier ("Introduction to Algorithms", ISBN 0-262-03141-8),
	 * donc nous prenons le nombre premier le plus proche de 20 000.
	 */
	static constexpr auto TAILLE_MAX = 19997;

	/**
	 * Fonction de hachage repris de "Optimized Spatial Hashing for Collision
	 * Detection of Deformable Objects"
	 * http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
	 *
	 * Pour calculer l'empreinte d'une position, nous considérons la partie
	 * entière de celle-ci. Par exemple, le vecteur position <0.1, 0.2, 0.3>
	 * deviendra <0, 0, 0> ; de même pour le vecteur <0.4, 0.5, 0.6>. Ainsi,
	 * toutes les positions se trouvant entre <0, 0, 0> et
	 * <0.99.., 0.99.., 0.99..> seront dans la même alvéole.
	 */
	std::size_t fonction_empreinte(dls::math::vec3f const &position);

	/**
	 * Ajoute la posistion spécifiée dans le vecteur des positions ayant la même
	 * empreinte que celle-ci.
	 */
	void ajoute(dls::math::vec3f const &position);

	/**
	 * Retourne un vecteur contenant les positions ayant la même empreinte que
	 * la position passée en paramètre.
	 */
	std::vector<dls::math::vec3f> const &particules(dls::math::vec3f const &position);

	/**
	 * Retourne le nombre d'alvéoles présentes dans la table de hachage.
	 */
	size_t taille() const;
};

/**
 * Retourne vrai si le point se trouve à une distance supérieure à 'distance' de
 * tous les autres points dans la table de hachage.
 */
bool verifie_distance_minimal(
		HachageSpatial &hachage_spatial,
		dls::math::vec3f const &point,
		float distance);
