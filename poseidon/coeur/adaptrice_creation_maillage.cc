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

#include "maillage.h"

void AdaptriceCreationMaillage::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	maillage->ajoute_sommet(numero7::math::vec3f(x, y, z));
}

void AdaptriceCreationMaillage::ajoute_normal(const float x, const float y, const float z)
{}

void AdaptriceCreationMaillage::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{}

void AdaptriceCreationMaillage::ajoute_parametres_sommet(const float x, const float y, const float z)
{}

void AdaptriceCreationMaillage::ajoute_polygone(const int *index_sommet, const int *, const int *, int nombre)
{
	auto poly_3 = (nombre == 3) ? -1 : index_sommet[3];

	maillage->ajoute_quad(index_sommet[0], index_sommet[1], index_sommet[2], poly_3);
}

void AdaptriceCreationMaillage::ajoute_ligne(const int *index, int nombre)
{}

void AdaptriceCreationMaillage::ajoute_objet(const std::string &nom)
{}
