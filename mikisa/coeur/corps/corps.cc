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

#include "../attribut.h"

#include "groupes.h"

Corps::~Corps()
{
	reinitialise();
}

bool Corps::possede_attribut(const std::string &nom)
{
	return this->attribut(nom) != nullptr;
}

Attribut *Corps::ajoute_attribut(const std::string &nom, int type, int portee, size_t taille)
{
	auto attr = attribut(nom);

	if (attr == nullptr) {
		attr = new Attribut(nom, static_cast<type_attribut>(type), portee, taille);
		m_attributs.push_back(attr);
	}

	return attr;
}

void Corps::supprime_attribut(const std::string &nom)
{
	auto iter = std::find_if(m_attributs.begin(), m_attributs.end(),
							 [&](Attribut *attr)
	{
		return attr->nom() == nom;
	});

	m_attributs.erase(iter);
}

Attribut *Corps::attribut(const std::string &nom) const
{
	for (const auto &attr : m_attributs) {
		if (attr->nom() != nom) {
			continue;
		}

		return attr;
	}

	return nullptr;
}

GroupePolygone *Corps::ajoute_groupe_polygone(const std::string &nom)
{
	if (m_groupes_polygones.find(nom) != m_groupes_polygones.end()) {
		return nullptr;
	}

	auto groupe = new GroupePolygone;
	groupe->nom = nom;

	m_groupes_polygones[nom] = groupe;

	return groupe;
}

int Corps::ajoute_point(float x, float y, float z)
{
	auto index = index_point(x, y, z);

	if (index != -1) {
		return index;
	}

	auto point = new Point3D();
	point->x = x;
	point->y = y;
	point->z = z;

	m_points.pousse(point);

	return m_points.taille() - 1;
}

int Corps::index_point(float x, float y, float z)
{
//	int i = 0;

//	for (Point3D *point : m_points.points()) {
//		if (point->x == x && point->y == y && point->z == z) {
//			return i;
//		}

//		++i;
//	};

	return -1;
}

void Corps::ajoute_polygone(Polygone *p)
{
	p->index = m_polys.taille();
	m_polys.pousse(p);
}

ListePoints3D *Corps::points()
{
	return &m_points;
}

const ListePoints3D *Corps::points() const
{
	return &m_points;
}

ListePolygones *Corps::polys()
{
	return &m_polys;
}

const ListePolygones *Corps::polys() const
{
	return &m_polys;
}

void Corps::reinitialise()
{
	m_points.reinitialise();
	m_polys.reinitialise();

	for (auto &attribut : m_attributs) {
		delete attribut;
	}

	m_attributs.clear();

	for (auto paire : m_groupes_polygones) {
		delete paire.second;
	}

	m_groupes_polygones.clear();
}

Corps *Corps::copie() const
{
	auto corps = new Corps();
	this->copie_vers(corps);
	return corps;
}

void Corps::copie_vers(Corps *corps) const
{
	/* copie les points */
	auto point_autre = corps->points();
	point_autre->reserve(points()->taille());

	for (Point3D *point : this->points()->points()) {
		auto p3d = new Point3D;
		p3d->x = point->x;
		p3d->y = point->y;
		p3d->z = point->z;
		point_autre->pousse(p3d);
	}

	/* copie les polygones */
	auto polys_autre = corps->polys();
	polys_autre->reserve(this->polys()->taille());

	for (Polygone *polygone : this->polys()->polys()) {
		auto poly = Polygone::construit(corps, polygone->type, polygone->nombre_sommets());

		for (int i = 0; i < polygone->nombre_sommets(); ++i) {
			poly->ajoute_sommet(polygone->index_point(i));
		}
	}

	/* copie les attributs */
	for (Attribut *attr : this->m_attributs) {
		Attribut *attr_corps = new Attribut(*attr);

		corps->m_attributs.push_back(attr_corps);
	}

	/* copie les groupes */
	/* À FAIRE */
}
