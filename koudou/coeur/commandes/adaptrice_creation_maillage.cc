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

#include "adaptrice_creation_maillage.h"

#include "../maillage.h"

void AdaptriceChargementMaillage::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	m_sommets.push_back(numero7::math::vec3d(x, y, z));
}

void AdaptriceChargementMaillage::ajoute_normal(const float x, const float y, const float z)
{}

void AdaptriceChargementMaillage::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{}

void AdaptriceChargementMaillage::ajoute_parametres_sommet(const float x, const float y, const float z)
{}

void AdaptriceChargementMaillage::ajoute_polygone(const int *index_sommet, const int *, const int *, int nombre)
{
		maillage->ajoute_triangle(
					m_sommets[index_sommet[0]],
				m_sommets[index_sommet[1]],
				m_sommets[index_sommet[2]]);

	if (nombre == 4) {
		maillage->ajoute_triangle(
					m_sommets[index_sommet[0]],
				m_sommets[index_sommet[2]],
				m_sommets[index_sommet[3]]);
	}
}

void AdaptriceChargementMaillage::ajoute_ligne(const int *index, int nombre)
{}

void AdaptriceChargementMaillage::ajoute_objet(const std::string &nom)
{}
