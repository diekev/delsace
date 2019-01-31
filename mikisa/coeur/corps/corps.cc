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

bool Corps::possede_attribut(const std::string &nom_attribut)
{
	return this->attribut(nom_attribut) != nullptr;
}

Attribut *Corps::ajoute_attribut(const std::string &nom_attribut, type_attribut type_, portee_attr portee, size_t taille_)
{
	auto attr = attribut(nom_attribut);

	if (attr == nullptr) {
		attr = new Attribut(nom_attribut, type_, portee, taille_);
		m_attributs.push_back(attr);
	}

	return attr;
}

void Corps::supprime_attribut(const std::string &nom_attribut)
{
	auto iter = std::find_if(m_attributs.begin(), m_attributs.end(),
							 [&](Attribut *attr)
	{
		return attr->nom() == nom_attribut;
	});

	delete *iter;

	m_attributs.erase(iter);
}

Attribut *Corps::attribut(const std::string &nom_attribut) const
{
	for (auto const &attr : m_attributs) {
		if (attr->nom() != nom_attribut) {
			continue;
		}

		return attr;
	}

	return nullptr;
}

GroupePrimitive *Corps::ajoute_groupe_primitive(const std::string &nom_attribut)
{
	if (m_groupes_prims.find(nom_attribut) != m_groupes_prims.end()) {
		return nullptr;
	}

	auto groupe = new GroupePrimitive;
	groupe->nom = nom_attribut;

	m_groupes_prims[nom_attribut] = groupe;

	return groupe;
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

	return m_points.taille() - 1;
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
	p->index = m_prims.taille();
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

	for (auto paire : m_groupes_prims) {
		delete paire.second;
	}

	m_groupes_prims.clear();
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
	auto prims_autre = corps->prims();
	prims_autre->reserve(this->prims()->taille());

	for (Primitive *prim : this->prims()->prims()) {
		if (prim->type_prim() == type_primitive::POLYGONE) {
			auto polygone = dynamic_cast<Polygone *>(prim);
			auto poly = Polygone::construit(corps, polygone->type, polygone->nombre_sommets());

			/* Nous obtenons des crashs lors des copies car l'index devient
			 * différent ou n'est pas correctement initialisé ? */
			poly->index = polygone->index;

			for (size_t i = 0; i < polygone->nombre_sommets(); ++i) {
				poly->ajoute_sommet(polygone->index_point(i));
			}
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
