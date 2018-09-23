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

#include "collection.h"

#include "courbes.h"
#include "maillage.h"
#include "nuage_points.h"

Collection::~Collection()
{
	reinitialise(true);
}

Maillage *Collection::cree_maillage()
{
	auto maillage = new Maillage();
	this->ajoute_corps(maillage);
	return maillage;
}

Courbes *Collection::cree_courbes()
{
	auto courbes = new Courbes();
	this->ajoute_corps(courbes);
	return courbes;
}

NuagePoints *Collection::cree_points()
{
	auto points = new NuagePoints();
	this->ajoute_corps(points);
	return points;
}

void Collection::ajoute_corps(Corps *corps)
{
	m_corps.push_back(corps);
}

void Collection::reinitialise(bool supprime_corps)
{
	if (supprime_corps) {
		for (auto corps : m_corps) {
			delete corps;
		}
	}

	m_corps.clear();
}

void Collection::transfers_corps_a(Collection &autre)
{
	autre.reserve_supplement(m_corps.size());

	for (auto corps : m_corps) {
		autre.ajoute_corps(corps);
	}

	m_corps.clear();
}

void Collection::reserve_supplement(const size_t nombre)
{
	m_corps.reserve(m_corps.size() + nombre);
}

Collection::plage_corps Collection::plage()
{
	return plage_corps(m_corps.begin(), m_corps.end());
}

Collection::plage_corps_const Collection::plage() const
{
	return plage_corps_const(m_corps.cbegin(), m_corps.cend());
}
