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

#include "corps.h"

#include <algorithm>

#include "bibliotheques/outils/definitions.hh"

#include "../attribut.h"

#include "groupes.h"

Corps::~Corps()
{
	reinitialise();
}

bool Corps::possede_attribut(std::string const &nom_attribut)
{
	return this->attribut(nom_attribut) != nullptr;
}

Attribut *Corps::ajoute_attribut(
		std::string const &nom_attribut,
		type_attribut type_,
		portee_attr portee,
		bool force_vide)
{
	auto attr = attribut(nom_attribut);

	if (attr == nullptr) {
		auto taille_attrib = 0l;

		if (!force_vide) {
			auto liste_points = this->points();
			auto liste_prims = this->prims();

			switch (portee) {
				case portee_attr::POINT:
					taille_attrib = liste_points->taille();
					break;
				case portee_attr::PRIMITIVE:
					taille_attrib = liste_prims->taille();
					break;
				case portee_attr::VERTEX:
					for (auto i = 0; i < liste_prims->taille(); ++i) {
						auto prim = liste_prims->prim(i);
						if (prim->type_prim() != type_primitive::POLYGONE) {
							continue;
						}

						auto poly = dynamic_cast<Polygone *>(prim);
						taille_attrib += poly->nombre_sommets();
					}
					break;
				case portee_attr::GROUPE:
					taille_attrib = static_cast<long>(this->m_groupes_prims.size());
					break;
				case portee_attr::CORPS:
					taille_attrib = 1;
					break;
			}
		}

		attr = new Attribut(nom_attribut, type_, portee, taille_attrib);
		m_attributs.push_back(attr);
	}

	return attr;
}

void Corps::supprime_attribut(std::string const &nom_attribut)
{
	auto iter = std::find_if(m_attributs.begin(), m_attributs.end(),
							 [&](Attribut *attr)
	{
		return attr->nom() == nom_attribut;
	});

	delete *iter;

	m_attributs.erase(iter);
}

Attribut *Corps::attribut(std::string const &nom_attribut) const
{
	for (auto const &attr : m_attributs) {
		if (attr->nom() != nom_attribut) {
			continue;
		}

		return attr;
	}

	return nullptr;
}

size_t Corps::ajoute_point(float x, float y, float z)
{
	auto index = index_point(x, y, z);

	if (index != -1ul) {
		return index;
	}

	auto point = new Point3D();
	point->x = x;
	point->y = y;
	point->z = z;

	m_points.pousse(point);

	return static_cast<size_t>(m_points.taille()) - 1;
}

size_t Corps::index_point(float x, float y, float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
//	int i = 0;

//	for (Point3D *point : m_points.points()) {
//		if (point->x == x && point->y == y && point->z == z) {
//			return i;
//		}

//		++i;
//	};

	return -1ul;
}

void Corps::ajoute_primitive(Primitive *p)
{
	p->index = static_cast<size_t>(m_prims.taille());
	m_prims.pousse(p);
}

ListePoints3D *Corps::points()
{
	return &m_points;
}

const ListePoints3D *Corps::points() const
{
	return &m_points;
}

ListePrimitives *Corps::prims()
{
	return &m_prims;
}

const ListePrimitives *Corps::prims() const
{
	return &m_prims;
}

void Corps::reinitialise()
{
	m_points.reinitialise();
	m_prims.reinitialise();

	for (auto &attribut : m_attributs) {
		delete attribut;
	}

	m_attributs.clear();

	m_groupes_prims.clear();
	m_groupes_points.clear();
}

Corps *Corps::copie() const
{
	auto corps = new Corps();
	this->copie_vers(corps);
	return corps;
}

void Corps::copie_vers(Corps *corps) const
{
	/* copie la transformation */
	corps->transformation = this->transformation;
	corps->pivot = this->pivot;
	corps->echelle = this->echelle;
	corps->position = this->position;
	corps->rotation = this->rotation;

	/* copie les points */
	corps->m_points = this->m_points;

	/* copie les primitives */
	corps->m_prims = this->m_prims;

	/* copie les attributs */
	for (Attribut *attr : this->m_attributs) {
		Attribut *attr_corps = new Attribut(*attr);

		corps->m_attributs.push_back(attr_corps);
	}

	/* copie les groupes */
	corps->m_groupes_points.reserve(this->m_groupes_points.size());

	for (auto groupe : this->m_groupes_points) {
		corps->m_groupes_points.push_back(groupe);
	}

	corps->m_groupes_prims.reserve(this->m_groupes_prims.size());

	for (auto groupe : this->m_groupes_prims) {
		corps->m_groupes_prims.push_back(groupe);
	}
}

Corps::plage_attributs Corps::attributs()
{
	return plage_attributs(m_attributs.begin(), m_attributs.end());
}

Corps::plage_const_attributs Corps::attributs() const
{
	return plage_const_attributs(m_attributs.cbegin(), m_attributs.cend());
}

/* ************************************************************************** */

GroupePoint *Corps::ajoute_groupe_point(const std::string &nom_groupe)
{
	auto ptr_groupe = groupe_point(nom_groupe);

	if (ptr_groupe != nullptr) {
		return ptr_groupe;
	}

	auto groupe = GroupePoint{};
	groupe.nom = nom_groupe;

	m_groupes_points.push_back(groupe);

	return &m_groupes_points.back();
}

GroupePoint *Corps::groupe_point(const std::string &nom_groupe) const
{
	auto iter = std::find_if(m_groupes_points.begin(), m_groupes_points.end(),
							 [&](GroupePoint const &groupe)
	{
				return groupe.nom == nom_groupe;
	});

	if (iter != m_groupes_points.end()) {
		auto index = static_cast<size_t>(std::distance(m_groupes_points.begin(), iter));
		return const_cast<GroupePoint *>(&m_groupes_points[index]);
	}

	return nullptr;
}

Corps::plage_grp_pnts Corps::groupes_points()
{
	return plage_grp_pnts(m_groupes_points.begin(), m_groupes_points.end());
}

Corps::plage_const_grp_pnts Corps::groupes_points() const
{
	return plage_const_grp_pnts(m_groupes_points.cbegin(), m_groupes_points.cend());
}

/* ************************************************************************** */

GroupePrimitive *Corps::ajoute_groupe_primitive(std::string const &nom_groupe)
{
	auto ptr_groupe = groupe_primitive(nom_groupe);

	if (ptr_groupe != nullptr) {
		return ptr_groupe;
	}

	auto groupe = GroupePrimitive{};
	groupe.nom = nom_groupe;

	m_groupes_prims.push_back(groupe);

	return &m_groupes_prims.back();
}

GroupePrimitive *Corps::groupe_primitive(const std::string &nom_groupe) const
{
	auto iter = std::find_if(m_groupes_prims.begin(), m_groupes_prims.end(),
							 [&](GroupePrimitive const &groupe)
	{
				return groupe.nom == nom_groupe;
	});

	if (iter != m_groupes_prims.end()) {
		auto index = static_cast<size_t>(std::distance(m_groupes_prims.begin(), iter));
		return const_cast<GroupePrimitive *>(&m_groupes_prims[index]);
	}

	return nullptr;
}

Corps::plage_grp_prims Corps::groupes_prims()
{
	return plage_grp_prims(m_groupes_prims.begin(), m_groupes_prims.end());
}

Corps::plage_const_grp_prims Corps::groupes_prims() const
{
	return plage_const_grp_prims(m_groupes_prims.cbegin(), m_groupes_prims.cend());
}
