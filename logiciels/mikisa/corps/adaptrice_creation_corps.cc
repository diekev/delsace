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

	auto point = dls::math::vec3f(x, y, z);
	corps->points_pour_ecriture()->pousse(point);
}

void AdaptriceCreationCorps::ajoute_normal(const float x, const float y, const float z)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::R32, 3, portee_attr::POINT, true);
	}

	auto idx = attribut_normal->taille();
	attribut_normal->redimensionne(attribut_normal->taille() + 1);
	assigne(attribut_normal->r32(idx), dls::math::vec3f(x, y, z));
}

void AdaptriceCreationCorps::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	INUTILISE(w);

	uvs.pousse(dls::math::vec2f(u, v));
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

	auto poly = corps->ajoute_polygone(type_polygone::FERME, nombre);

	if (index_uvs != nullptr && attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::R32, 2, portee_attr::VERTEX);
	}

	for (long i = 0; i < nombre; ++i) {
		auto idx = corps->ajoute_sommet(poly, index_sommet[i]);

		if (attribut_uvs != nullptr) {
			assigne(attribut_uvs->r32(idx), uvs[index_uvs[i]]);
		}
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
				attribut_normal_polys = corps->ajoute_attribut("N_polys", type_attribut::R32, 3, portee_attr::PRIMITIVE);
			}

			copie_attribut(attribut_normal, index_normaux[0], attribut_normal_polys, poly->index);
		}
	}

	for (GroupePrimitive *groupe : groupes_courant) {
		groupe->ajoute_index(poly->index);
	}
}

void AdaptriceCreationCorps::ajoute_ligne(const int *index, size_t nombre)
{
	INUTILISE(index);
	INUTILISE(nombre);
}

void AdaptriceCreationCorps::ajoute_objet(dls::chaine const &nom)
{
	corps->nom = nom;
}

void AdaptriceCreationCorps::reserve_polygones(long const nombre)
{
	corps->prims()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_sommets(long const nombre)
{
	corps->points_pour_ecriture()->reserve(nombre);
}

void AdaptriceCreationCorps::reserve_normaux(long const nombre)
{
	if (attribut_normal == nullptr) {
		attribut_normal = corps->ajoute_attribut("N", type_attribut::R32, 3, portee_attr::POINT, true);
		attribut_normal->reserve(nombre);
	}
}

void AdaptriceCreationCorps::reserve_uvs(long const nombre)
{
	uvs.reserve(nombre);
}

void AdaptriceCreationCorps::groupes(dls::tableau<dls::chaine> const &noms)
{
	groupes_courant.efface();

	for (auto const &nom : noms) {
		auto groupe = corps->ajoute_groupe_primitive(nom);
		groupes_courant.pousse(groupe);
	}
}

void AdaptriceCreationCorps::groupe_nuancage(const int index)
{
	INUTILISE(index);
}
