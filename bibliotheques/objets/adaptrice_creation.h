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

#include <string>
#include <vector>

namespace objets {

class AdaptriceCreationObjet {
public:
	virtual ~AdaptriceCreationObjet() = default;

	virtual void ajoute_sommet(const float x, const float y, const float z, const float w = 1.0f) = 0;

	virtual void ajoute_normal(const float x, const float y, const float z) = 0;

	virtual void ajoute_coord_uv_sommet(const float u, const float v, const float w = 0.0f) = 0;

	virtual void ajoute_parametres_sommet(const float x, const float y, const float z) = 0;

	virtual void ajoute_polygone(const int *index_sommet, const int *index_uv, const int *index_normal, long nombre) = 0;

	virtual void ajoute_ligne(const int *index, size_t nombre) = 0;

	virtual void ajoute_objet(std::string const &nom) = 0;

	virtual void reserve_polygones(long const nombre) = 0;

	virtual void reserve_sommets(long const nombre) = 0;

	virtual void reserve_normaux(long const nombre) = 0;

	virtual void reserve_uvs(long const nombre) = 0;

	virtual void groupes(std::vector<std::string> const &noms) = 0;

	virtual void groupe_nuancage(const int index) = 0;
};

}  /* namespace objets */
