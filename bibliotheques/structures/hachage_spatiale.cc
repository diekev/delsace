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

#include "hachage_spatiale.hh"

size_t HachageSpatial::fonction_empreinte(dls::math::vec3f const &position)
{
	return static_cast<std::size_t>(
				static_cast<int>(position.x) * 73856093
				^ static_cast<int>(position.y) * 19349663
				^ static_cast<int>(position.z) * 83492791) % TAILLE_MAX;
}

void HachageSpatial::ajoute(dls::math::vec3f const &position)
{
	auto const empreinte = fonction_empreinte(position);
	m_tableau[empreinte].push_back(position);
}

std::vector<dls::math::vec3f> const &HachageSpatial::particules(dls::math::vec3f const &position)
{
	auto const empreinte = fonction_empreinte(position);
	return m_tableau[empreinte];
}

size_t HachageSpatial::taille() const
{
	return m_tableau.size();
}

bool verifie_distance_minimal(HachageSpatial &hachage_spatial, const dls::math::vec3f &point, float distance)
{
	auto const points = hachage_spatial.particules(point);

	for (auto p = 0ul; p < points.size(); ++p) {
		if (longueur(point - points[p]) < distance) {
			return false;
		}
	}

	return true;
}
