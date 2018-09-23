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

#include "../attribut.h"

#include "groupes.h"
#include "maillage.h"

/* ************************************************************************** */

void AdaptriceCreationMaillage::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	maillage->ajoute_sommet(numero7::math::vec3f(x, y, z));
}

void AdaptriceCreationMaillage::ajoute_normal(const float x, const float y, const float z)
{
//	maillage->ajoute_normal(numero7::math::vec3f(x, y, z));

	if (attribut_normal == nullptr) {
		attribut_normal = maillage->ajoute_attribut("N", type_attribut::ATTRIBUT_VEC3, ATTR_PORTEE_POINT, 0);
	}

	attribut_normal->pousse_vec3(numero7::math::vec3f(x, y, z));
}

void AdaptriceCreationMaillage::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
//	maillage->ajoute_uv(numero7::math::vec2f(u, v));

	if (attribut_uvs == nullptr) {
		attribut_uvs = maillage->ajoute_attribut("UV", type_attribut::ATTRIBUT_VEC2, ATTR_PORTEE_POLYGONE_POINT, 0);
	}

	attribut_uvs->pousse_vec2(numero7::math::vec2f(u, v));
}

void AdaptriceCreationMaillage::ajoute_parametres_sommet(const float x, const float y, const float z)
{}

void AdaptriceCreationMaillage::ajoute_polygone(const int *index_sommet, const int *index_uvs, const int *index_normaux, int nombre)
{
	auto poly_3 = (nombre == 3) ? -1 : index_sommet[3];

	auto poly = maillage->ajoute_quad(index_sommet[0], index_sommet[1], index_sommet[2], poly_3);

	if (index_normaux) {
		for (int i = 0; i < nombre; ++i) {
			poly->normaux[i] = index_normaux[i];
		}
	}

	if (index_uvs) {
		for (int i = 0; i < nombre; ++i) {
			poly->uvs[i] = index_uvs[i];
		}
	}

//	std::vector<std::string> noms;

//	for (const auto &nom : noms) {
//		maillage->groupe(nom).push_back(poly->index);
//	}
}

void AdaptriceCreationMaillage::ajoute_ligne(const int *index, int nombre)
{}

void AdaptriceCreationMaillage::ajoute_objet(const std::string &nom)
{
	maillage->nom = nom;
}

void AdaptriceCreationMaillage::reserve_polygones(const size_t nombre)
{
	maillage->reserve_polygones(nombre);
}

void AdaptriceCreationMaillage::reserve_sommets(const size_t nombre)
{
	maillage->reserve_sommets(nombre);
}

void AdaptriceCreationMaillage::reserve_normaux(const size_t nombre)
{
	if (attribut_normal == nullptr) {
		attribut_normal = maillage->ajoute_attribut("N", type_attribut::ATTRIBUT_VEC3, ATTR_PORTEE_POINT, 0);
		attribut_normal->reserve(nombre);
	}
}

void AdaptriceCreationMaillage::reserve_uvs(const size_t nombre)
{
	if (attribut_uvs == nullptr) {
		attribut_uvs = maillage->ajoute_attribut("UV", type_attribut::ATTRIBUT_VEC2, ATTR_PORTEE_POLYGONE_POINT, 0);
		attribut_uvs->reserve(nombre);
	}
}

void AdaptriceCreationMaillage::groupes(const std::vector<std::string> &noms)
{
	/* À FAIRE : création de groupes nommés de polygones ; ici nous définissons
	 * quels groupes sont actifs. */

//	std::map<std::string, std::vector<int>> m_groupes_polys;
}

void AdaptriceCreationMaillage::groupe_nuancage(const int index)
{}

/* ************************************************************************** */

void AdaptriceCreationCorps::ajoute_sommet(const float x, const float y, const float z, const float w)
{
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

	attribut_normal->pousse_vec3(numero7::math::vec3f(x, y, z));
}

void AdaptriceCreationCorps::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	if (attribut_uvs == nullptr) {
		attribut_uvs = corps->ajoute_attribut("UV", type_attribut::ATTRIBUT_VEC2, ATTR_PORTEE_POLYGONE_POINT, 0);
	}

	attribut_uvs->pousse_vec2(numero7::math::vec2f(u, v));
}

void AdaptriceCreationCorps::ajoute_parametres_sommet(const float x, const float y, const float z)
{}

void AdaptriceCreationCorps::ajoute_polygone(const int *index_sommet, const int *index_uvs, const int *index_normaux, int nombre)
{
	auto poly = Polygone::construit(corps, POLYGONE_FERME, nombre);

	for (int i = 0; i < nombre; ++i) {
		poly->ajoute_sommet(index_sommet[i]);
	}

	for (GroupePolygone *groupe : groupes_courant) {
		groupe->ajoute_primitive(poly->index);
	}
}

void AdaptriceCreationCorps::ajoute_ligne(const int *index, int nombre)
{}

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

	for (const auto &nom : noms) {
		auto groupe = corps->ajoute_groupe_polygone(nom);
		groupes_courant.push_back(groupe);
	}
}

void AdaptriceCreationCorps::groupe_nuancage(const int index)
{}
