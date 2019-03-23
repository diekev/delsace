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

#include "limites_corps.hh"

#include "bibliotheques/outils/constantes.h"

#include "../corps/corps.h"

static auto initialise_limites3f()
{
	auto limites = limites3f{};
	limites.min = dls::math::vec3f( constantes<float>::INFINITE);
	limites.max = dls::math::vec3f(-constantes<float>::INFINITE);
	return limites;
}

limites3f calcule_limites_mondiales_corps(Corps const &corps)
{
	auto limites = initialise_limites3f();

	auto const &points = corps.points();

	for (auto i = 0; i < points->taille(); ++i) {
		auto point = corps.point_transforme(i);
		dls::math::extrait_min_max(point, limites.min, limites.max);
	}

	return limites;
}

limites3f calcule_limites_locales_corps(const Corps &corps)
{
	auto limites = initialise_limites3f();

	auto const &points = corps.points();

	for (auto i = 0; i < points->taille(); ++i) {
		auto point = points->point(i);
		dls::math::extrait_min_max(point, limites.min, limites.max);
	}

	return limites;
}
