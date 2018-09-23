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

size_t UsineOperatrice::register_type(const std::string &name, const DescOperatrice &desc)
{
	const auto iter = m_map.find(name);
	assert(iter == m_map.end());

	m_map[name] = desc;
	return num_entries();
}

OperatriceImage *UsineOperatrice::operator()(const std::string &name, Noeud *node)
{
	const auto iter = m_map.find(name);
	assert(iter != m_map.end());

	const DescOperatrice &desc = iter->second;

	return desc.build_operator(node);
}

std::vector<DescOperatrice> UsineOperatrice::keys() const
{
	std::vector<DescOperatrice> v;
	v.reserve(num_entries());

	for (const auto &entry : m_map) {
		v.push_back(entry.second);
	}

	return v;
}

bool UsineOperatrice::registered(const std::string &key) const
{
	return (m_map.find(key) != m_map.end());
}
