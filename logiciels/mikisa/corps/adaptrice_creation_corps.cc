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

#include "biblinternes/outils/definitions.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "attribut.h"
#include "corps.h"
#include "groupes.h"
#include "listes.h"

/* ************************************************************************** */

void AdaptriceCreationCorps::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	INUTILISE(w);

	auto point = memoire::loge<Point3D>("Point3D");
	point->x = x;
	point->y = y;
	point->z = z;

	corps->points()->pousse(point);
}

void AdaptriceCreationCorps::ajoute_normal(const float x, const float y, const float z)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT, true);
	}

	attribut_normal->pousse(dls::math::vec3f(x, y, z));
}

void AdaptriceCreationCorps::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	INUTILISE(w);

	if (attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::VEC2, portee_attr::VERTEX, true);
	}

	attribut_uvs->pousse(dls::math::vec2f(u, v));
}

void AdaptriceCreationCorps::ajoute_parametres_sommet(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceCreationCorps::ajoute_polygone(const int *index_sommet, const int *index_uvs, const int *index_normaux, long nombre)
{
	INUTILISE(index_uvs);

	auto poly = Polygone::construit(corps, type_polygone::FERME, nombre);

	for (long i = 0; i < nombre; ++i) {
		poly->ajoute_sommet(index_sommet[i]);
	}

	if (index_normaux != nullptr) {
		auto normaux_polys = true;

		for (auto i = 1; i < nombre; ++i) {
			if (index_normaux[i - 1] != index_normaux[i]) {
				normaux_polys = false;
			}
		}

		if (normaux_polys) {
			if (attribut_normal_polys == nullptr) {
				attribut_normal_polys = corps->ajoute_attribut("N_polys", type_attribut::VEC3, portee_attr::PRIMITIVE, true);
			}

			attribut_normal_polys->pousse(attribut_normal->vec3(index_normaux[0]));
		}
	}

	for (GroupePrimitive *groupe : groupes_courant) {
		groupe->ajoute_primitive(poly->index);
	}
}

void AdaptriceCreationCorps::ajoute_ligne(const int *index, size_t nombre)
{
	INUTILISE(index);
	INUTILISE(nombre);
}

void AdaptriceCreationCorps::ajoute_objet(std::string const &nom)
{
	corps->nom = nom;
}

void AdaptriceCreationCorps::reserve_polygones(long const nombre)
{
	corps->prims()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_sommets(long const nombre)
{
	corps->points()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_normaux(long const nombre)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT, true);
		attribut_normal->reserve(nombre);
	}
}

void AdaptriceCreationCorps::reserve_uvs(long const nombre)
{
	if (attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::VEC2, portee_attr::VERTEX, true);
		attribut_uvs->reserve(nombre);
	}
}

void AdaptriceCreationCorps::groupes(dls::tableau<std::string> const &noms)
{
	groupes_courant.clear();

	for (auto const &nom : noms) {
		auto groupe = corps->ajoute_groupe_primitive(nom);
		groupes_courant.pousse(groupe);
	}
}

void AdaptriceCreationCorps::groupe_nuancage(const int index)
{
	INUTILISE(index);
}
