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

#include "boite_englobante.h"

#include "types.h"

BoiteEnglobante::BoiteEnglobante(const dls::math::point3d &point)
	: min(point)
	, max(point)
{}

BoiteEnglobante::BoiteEnglobante(const dls::math::point3d &p1, const dls::math::point3d &p2)
	: min(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z))
	, max(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z))
{}

bool BoiteEnglobante::chevauchement(const BoiteEnglobante &boite)
{
	const auto x = (max[0] >= boite.min[0]) && (min[0] <= boite.max[0]);
	const auto y = (max[1] >= boite.min[1]) && (min[1] <= boite.max[1]);
	const auto z = (max[2] >= boite.min[2]) && (min[2] <= boite.max[2]);

	return x && y && z;
}

bool BoiteEnglobante::contient(const dls::math::point3d &point)
{
	const auto x = (point[0] >= min[0]) && (point[0] <= max[0]);
	const auto y = (point[1] >= min[1]) && (point[1] <= max[1]);
	const auto z = (point[2] >= min[2]) && (point[2] <= max[2]);

	return x && y && z;
}

void BoiteEnglobante::etend(double delta)
{
	for (size_t i = 0; i < 3; ++i) {
		min[i] -= delta;
		max[i] += delta;
	}
}

double BoiteEnglobante::aire_surface() const
{
	const auto d = max - min;
	return 2.0 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

double BoiteEnglobante::volume() const
{
	const auto d = max - min;
	return (d.x * d.y * d.z);
}

int BoiteEnglobante::ampleur_maximale() const
{
	const auto diagonale = max - min;

	if (diagonale.x > diagonale.y && diagonale.x > diagonale.z) {
		return 0;
	}

	if (diagonale.y > diagonale.z) {
		return 1;
	}

	return 2;
}

dls::math::vec3d BoiteEnglobante::decalage(const dls::math::point3d &point)
{
	return dls::math::vec3d(
				(point[0] - min[0]) / (max[0] - min[0]),
			(point[1] - min[1]) / (max[1] - min[1]),
			(point[2] - min[2]) / (max[2] - min[2]));
}

BoiteEnglobante unie(const BoiteEnglobante &boite, const dls::math::point3d &point)
{
	BoiteEnglobante resultat;

	for (size_t i = 0; i < 3; ++i) {
		resultat.min[i] = std::min(boite.min[i], point[i]);
		resultat.max[i] = std::max(boite.max[i], point[i]);
	}

	return resultat;
}

BoiteEnglobante unie(const BoiteEnglobante &boite1, const BoiteEnglobante &boite2)
{
	BoiteEnglobante resultat;

	for (size_t i = 0; i < 3; ++i) {
		resultat.min[i] = std::min(boite1.min[i], boite2.min[i]);
		resultat.max[i] = std::max(boite1.max[i], boite2.max[i]);
	}

	return resultat;
}

/* Algorithme issu de
 * https://tavianator.com/fast-branchless-raybounding-box-entresections-part-2-nans/
 */
bool entresecte_boite(const BoiteEnglobante &boite, const Rayon &rayon)
{
	auto t1 = (boite.min[0] - rayon.origine[0]) * rayon.inverse_direction[0];
	auto t2 = (boite.max[0] - rayon.origine[0]) * rayon.inverse_direction[0];

	auto tmin = std::min(t1, t2);
	auto tmax = std::max(t1, t2);

	for (size_t i = 1; i < 3; ++i) {
		t1 = (boite.min[i] - rayon.origine[i]) * rayon.inverse_direction[i];
		t2 = (boite.max[i] - rayon.origine[i]) * rayon.inverse_direction[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	return tmax > std::max(tmin, 0.0);
}
