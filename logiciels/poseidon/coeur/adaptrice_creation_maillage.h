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

#include "biblinternes/objets/adaptrice_creation.h"

class Maillage;

class AdaptriceCreationMaillage : public objets::AdaptriceCreationObjet {
public:
	void ajoute_sommet(const float x, const float y, const float z, const float w = 1.0f) override;

	void ajoute_normal(const float x, const float y, const float z) override;

	void ajoute_coord_uv_sommet(const float u, const float v, const float w = 0.0f) override;

	void ajoute_parametres_sommet(const float x, const float y, const float z) override;

	void ajoute_polygone(const int *index_sommet, const int */*index_uv*/, const int */*index_normal*/, long nombre) override;

	void ajoute_ligne(const int *index, size_t nombre) override;

	void ajoute_objet(const dls::chaine &nom) override;

	void reserve_polygones(long const nombre) override {}

	void reserve_sommets(long const nombre) override {}

	void reserve_normaux(long const nombre) override {}

	void reserve_uvs(long const nombre) override {}

	void groupes(dls::tableau<dls::chaine> const &noms) override {}

	void groupe_nuancage(const int index) override {}

	Maillage *maillage{};
};
