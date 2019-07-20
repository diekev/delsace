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

#include "biblinternes/outils/definitions.h"

#include "../maillage.h"

void AdaptriceChargementMaillage::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	INUTILISE(w);
	m_sommets.pousse(dls::math::vec3d(x, y, z));
}

void AdaptriceChargementMaillage::ajoute_normal(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceChargementMaillage::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	INUTILISE(u);
	INUTILISE(v);
	INUTILISE(w);
}

void AdaptriceChargementMaillage::ajoute_parametres_sommet(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceChargementMaillage::ajoute_polygone(const int *index_sommet, const int *, const int *, long nombre)
{
	maillage->ajoute_triangle(
				m_sommets[static_cast<size_t>(index_sommet[0])],
			m_sommets[static_cast<size_t>(index_sommet[1])],
			m_sommets[static_cast<size_t>(index_sommet[2])]);

	if (nombre == 4) {
		maillage->ajoute_triangle(
					m_sommets[static_cast<size_t>(index_sommet[0])],
				m_sommets[static_cast<size_t>(index_sommet[2])],
				m_sommets[static_cast<size_t>(index_sommet[3])]);
	}
}

void AdaptriceChargementMaillage::ajoute_ligne(const int *index, size_t nombre)
{
	INUTILISE(index);
	INUTILISE(nombre);
}

void AdaptriceChargementMaillage::ajoute_objet(dls::chaine const &nom)
{
	INUTILISE(nom);
}

void AdaptriceChargementMaillage::reserve_polygones(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceChargementMaillage::reserve_sommets(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceChargementMaillage::reserve_normaux(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceChargementMaillage::reserve_uvs(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceChargementMaillage::groupes(const dls::tableau<dls::chaine> &noms)
{
	INUTILISE(noms);
}

void AdaptriceChargementMaillage::groupe_nuancage(const int index)
{
	INUTILISE(index);
}
