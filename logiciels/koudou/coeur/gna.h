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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <random>

/**
 * La classe GNA représente un simple Générateur de Nombre Aléatoire.
 */
class GNA {
	std::mt19937 m_gna;
	std::uniform_real_distribution<double> m_dist_double;
	std::uniform_int_distribution<unsigned int> m_dist_unsigned;

public:
	/**
	 * Construit un générateur avec la graine spécifiée.
	 */
	explicit GNA(unsigned int graine = 0);

	/**
	 * Change la graine du générateur avec celle spécifiée.
	 */
	void graine(unsigned int graine);

	/**
	 * Retourne un nombre aléatoire dans [0.0, 1.0].
	 */
	double nombre_aleatoire();

	/**
	 * Retourne un nombre entier aléatoire dans [0, 2^32].
	 */
	unsigned int entier_aleatoire();
};
