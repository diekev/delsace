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

#include "adaptrice_creation_corps.h"

#include "bibliotheques/outils/definitions.hh"

#include "../attribut.h"

#include "corps.h"
#include "groupes.h"
#include "listes.h"

/* ************************************************************************** */

void AdaptriceCreationCorps::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	INUTILISE(w);

	auto point = new Point3D;
	point->x = x;
	point->y = y;
	point->z = z;

	corps->points()->pousse(point);
}

void AdaptriceCreationCorps::ajoute_normal(const float x, const float y, const float z)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::ATTRIBUT_VEC3, ATTR_PORTEE_POINT, 0);
	}

	attribut_normal->pousse_vec3(dls::math::vec3f(x, y, z));
}

void AdaptriceCreationCorps::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	INUTILISE(w);

	if (attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::ATTRIBUT_VEC2, ATTR_PORTEE_POLYGONE_POINT, 0);
	}

	attribut_uvs->pousse_vec2(dls::math::vec2f(u, v));
}

void AdaptriceCreationCorps::ajoute_parametres_sommet(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceCreationCorps::ajoute_polygone(const int *index_sommet, const int *index_uvs, const int *index_normaux, size_t nombre)
{
	INUTILISE(index_uvs);
	INUTILISE(index_normaux);

	auto poly = Polygone::construit(corps, POLYGONE_FERME, nombre);

	for (size_t i = 0; i < nombre; ++i) {
		poly->ajoute_sommet(static_cast<size_t>(index_sommet[i]));
	}

	for (GroupePolygone *groupe : groupes_courant) {
		groupe->ajoute_primitive(poly->index);
	}
}

void AdaptriceCreationCorps::ajoute_ligne(const int *index, size_t nombre)
{
	INUTILISE(index);
	INUTILISE(nombre);
}

void AdaptriceCreationCorps::ajoute_objet(const std::string &nom)
{
	corps->nom = nom;
}

void AdaptriceCreationCorps::reserve_polygones(const size_t nombre)
{
	corps->polys()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_sommets(const size_t nombre)
{
	corps->points()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_normaux(const size_t nombre)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::ATTRIBUT_VEC3, ATTR_PORTEE_POINT, 0);
		attribut_normal->reserve(nombre);
	}
}

void AdaptriceCreationCorps::reserve_uvs(const size_t nombre)
{
	if (attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::ATTRIBUT_VEC2, ATTR_PORTEE_POLYGONE_POINT, 0);
		attribut_uvs->reserve(nombre);
	}
}

void AdaptriceCreationCorps::groupes(const std::vector<std::string> &noms)
{
	groupes_courant.clear();

	for (auto const &nom : noms) {
		auto groupe = corps->ajoute_groupe_polygone(nom);
		groupes_courant.push_back(groupe);
	}
}

void AdaptriceCreationCorps::groupe_nuancage(const int index)
{
	INUTILISE(index);
}
