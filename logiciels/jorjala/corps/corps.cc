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

#include "biblinternes/outils/definitions.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "groupes.h"
#include "sphere.hh"
#include "volume.hh"

Corps::~Corps()
{
	reinitialise();
}

bool Corps::possede_attribut(dls::chaine const &nom_attribut)
{
	return this->attribut(nom_attribut) != nullptr;
}

void Corps::ajoute_attribut(Attribut *attr)
{
	this->m_attributs.ajoute(*attr);
}

Attribut *Corps::ajoute_attribut(
		dls::chaine const &nom_attribut,
		type_attribut type_,
		int dimensions,
		portee_attr portee,
		bool force_vide)
{
	auto attr = attribut(nom_attribut);

	if (attr == nullptr) {
		auto taille_attrib = 0l;

		if (!force_vide) {
			auto liste_points = this->points_pour_lecture();
			auto liste_prims = this->prims();

			switch (portee) {
				case portee_attr::POINT:
					taille_attrib = liste_points.taille();
					break;
				case portee_attr::PRIMITIVE:
					taille_attrib = liste_prims->taille();
					break;
				case portee_attr::VERTEX:
					taille_attrib = this->nombre_sommets();
					break;
				case portee_attr::GROUPE:
					taille_attrib = this->m_groupes_prims.taille();
					break;
				case portee_attr::CORPS:
					taille_attrib = 1;
					break;
			}
		}

		auto nattr = Attribut(nom_attribut, type_, dimensions, portee, taille_attrib);
		m_attributs.ajoute(nattr);
		attr = &m_attributs.back();
	}

	return attr;
}

void Corps::supprime_attribut(dls::chaine const &nom_attribut)
{
	auto iter = std::find_if(m_attributs.debut(), m_attributs.fin(),
							 [&](Attribut const &attr)
	{
		return attr.nom() == nom_attribut;
	});

	if (iter == m_attributs.fin()) {
		return;
	}

	m_attributs.efface(iter);
}

Attribut *Corps::attribut(const dls::chaine &nom_attribut)
{
	for (auto &attr : m_attributs) {
		if (attr.nom() != nom_attribut) {
			continue;
		}

		return &attr;
	}

	return nullptr;
}

Attribut const *Corps::attribut(dls::chaine const &nom_attribut) const
{
	for (auto const &attr : m_attributs) {
		if (attr.nom() != nom_attribut) {
			continue;
		}

		return &attr;
	}

	return nullptr;
}

void Corps::ajoute_primitive(Primitive *p)
{
	p->index = m_prims.taille();
	m_prims.ajoute(p);
}

void Corps::copie_points(const Corps autre)
{
	m_points = autre.m_points;
}

AccesseusePointEcriture Corps::points_pour_ecriture()
{
	m_points.detache();
	return AccesseusePointEcriture(*this, m_points, transformation);
}

AccesseusePointLecture Corps::points_pour_lecture() const
{
	return AccesseusePointLecture(m_points, transformation);
}

ListePrimitives *Corps::prims()
{
	return &m_prims;
}

const ListePrimitives *Corps::prims() const
{
	return &m_prims;
}

Polygone *Corps::ajoute_polygone(type_polygone type_poly, long nombre_sommets)
{
	auto p = memoire::loge<Polygone>("Polygone");
	p->type = type_poly;
	p->reserve_sommets(nombre_sommets);

	ajoute_primitive(p);

	redimensionne_attributs(portee_attr::PRIMITIVE);

	return p;
}

long Corps::ajoute_sommet(Polygone *p, long idx_point)
{
	auto idx_sommet = m_nombre_sommets++;

	p->ajoute_point(idx_point, idx_sommet);

	redimensionne_attributs(portee_attr::VERTEX);

	return idx_sommet;
}

long Corps::nombre_sommets() const
{
	return m_nombre_sommets;
}

Sphere *Corps::ajoute_sphere(long idx_point, float rayon)
{
	auto sphere = memoire::loge<Sphere>("Sphère", idx_point, rayon);
	ajoute_primitive(sphere);
	return sphere;
}

void Corps::reinitialise()
{
	m_points.reinitialise();
	m_prims.reinitialise();
	m_nombre_sommets = 0;

	m_attributs.efface();

	m_groupes_points.efface();
	m_groupes_prims.efface();
}

Corps *Corps::copie() const
{
	auto corps = memoire::loge<Corps>("Corps");
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
	corps->m_nombre_sommets = this->nombre_sommets();

	/* copie les attributs */
	for (auto attr : this->m_attributs) {
		corps->m_attributs.ajoute(attr);
	}

	/* copie les groupes */
	for (auto groupe : this->m_groupes_points) {
		corps->m_groupes_points.ajoute(groupe);
	}

	for (auto groupe : this->m_groupes_prims) {
		corps->m_groupes_prims.ajoute(groupe);
	}
}

Corps::plage_attributs Corps::attributs()
{
	return plage_attributs(m_attributs.debut(), m_attributs.fin());
}

Corps::plage_const_attributs Corps::attributs() const
{
	return plage_const_attributs(m_attributs.debut(), m_attributs.fin());
}

/* ************************************************************************** */

GroupePoint *Corps::ajoute_groupe_point(const dls::chaine &nom_groupe)
{
	auto ptr_groupe = groupe_point(nom_groupe);

	if (ptr_groupe != nullptr) {
		return ptr_groupe;
	}

	auto groupe = GroupePoint{};
	groupe.nom = nom_groupe;

	m_groupes_points.ajoute(groupe);

	redimensionne_attributs(portee_attr::GROUPE);

	return &m_groupes_points.back();
}

GroupePoint *Corps::groupe_point(const dls::chaine &nom_groupe) const
{
	auto iter = std::find_if(m_groupes_points.debut(), m_groupes_points.fin(),
							 [&](GroupePoint const &groupe)
	{
				return groupe.nom == nom_groupe;
	});

	if (iter != m_groupes_points.fin()) {
		return const_cast<GroupePoint *>(&(*iter));
	}

	return nullptr;
}

Corps::plage_grp_pnts Corps::groupes_points()
{
	return plage_grp_pnts(m_groupes_points.debut(), m_groupes_points.fin());
}

Corps::plage_const_grp_pnts Corps::groupes_points() const
{
	return plage_const_grp_pnts(m_groupes_points.debut(), m_groupes_points.fin());
}

/* ************************************************************************** */

GroupePrimitive *Corps::ajoute_groupe_primitive(dls::chaine const &nom_groupe)
{
	auto ptr_groupe = groupe_primitive(nom_groupe);

	if (ptr_groupe != nullptr) {
		return ptr_groupe;
	}

	auto groupe = GroupePrimitive{};
	groupe.nom = nom_groupe;

	m_groupes_prims.ajoute(groupe);

	redimensionne_attributs(portee_attr::GROUPE);

	return &m_groupes_prims.back();
}

GroupePrimitive *Corps::groupe_primitive(const dls::chaine &nom_groupe) const
{
	auto iter = std::find_if(m_groupes_prims.debut(), m_groupes_prims.fin(),
							 [&](GroupePrimitive const &groupe)
	{
				return groupe.nom == nom_groupe;
	});

	if (iter != m_groupes_prims.fin()) {
		return const_cast<GroupePrimitive *>(&(*iter));
	}

	return nullptr;
}

Corps::plage_grp_prims Corps::groupes_prims()
{
	return plage_grp_prims(m_groupes_prims.debut(), m_groupes_prims.fin());
}

Corps::plage_const_grp_prims Corps::groupes_prims() const
{
	return plage_const_grp_prims(m_groupes_prims.debut(), m_groupes_prims.fin());
}

void Corps::redimensionne_attributs(portee_attr portee)
{
	for (auto &attr : m_attributs) {
		if (attr.portee == portee) {
			attr.redimensionne(attr.taille() + 1);
		}
	}
}

/* ************************************************************************** */

bool possede_volume(const Corps &corps)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() == type_primitive::VOLUME) {
			return true;
		}
	}

	return false;
}

bool possede_sphere(Corps const &corps)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() == type_primitive::SPHERE) {
			return true;
		}
	}

	return false;
}
