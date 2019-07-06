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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "usine_operatrice.h"

#include <cassert>

#include "operatrice_image.h"

size_t UsineOperatrice::enregistre_type(DescOperatrice const &desc)
{
	auto const iter = m_map.find(desc.name);
	assert(iter == m_map.end());

	m_map[desc.name] = desc;
	return num_entries();
}

OperatriceImage *UsineOperatrice::operator()(std::string const &name, Graphe &graphe_parent, Noeud *noeud)
{
	auto const iter = m_map.find(name);
	assert(iter != m_map.end());

	DescOperatrice const &desc = iter->second;

	auto operatrice = desc.build_operator(graphe_parent, noeud);
	operatrice->usine(this);

	return operatrice;
}

void UsineOperatrice::deloge(OperatriceImage *operatrice)
{
	auto const iter = m_map.find(operatrice->nom_classe());
	assert(iter != m_map.end());

	DescOperatrice &desc = iter->second;
	desc.supprime_operatrice(operatrice);
}

dls::tableau<DescOperatrice> UsineOperatrice::keys() const
{
	dls::tableau<DescOperatrice> v;
	v.reserve(static_cast<long>(num_entries()));

	for (auto const &entry : m_map) {
		v.pousse(entry.second);
	}

	return v;
}

bool UsineOperatrice::registered(std::string const &key) const
{
	return (m_map.find(key) != m_map.end());
}
