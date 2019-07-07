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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "vue_chaine.hh"

#include <iostream>

namespace dls {

vue_chaine::vue_chaine(const char *ptr)
	: m_ptr(ptr)
	, m_taille(static_cast<long>(std::strlen(m_ptr)))
{}

vue_chaine::vue_chaine(const char *ptr, long taille)
	: m_ptr(ptr)
	, m_taille(taille)
{}

const char &vue_chaine::operator[](long idx) const
{
	return m_ptr[idx];
}

long vue_chaine::taille() const
{
	return m_taille;
}

bool vue_chaine::est_vide() const
{
	return m_taille == 0;
}

bool operator<(const vue_chaine &c1, const vue_chaine &c2)
{
	auto taille = std::max(c1.taille(), c2.taille());
	return std::strncmp(&c1[0], &c2[0], static_cast<size_t>(taille)) < 0;
}

bool operator>(const vue_chaine &c1, const vue_chaine &c2)
{
	auto taille = std::max(c1.taille(), c2.taille());
	return std::strncmp(&c1[0], &c2[0], static_cast<size_t>(taille)) > 0;
}

std::ostream &operator<<(std::ostream &os, const vue_chaine &vc)
{
	for (auto i = 0; i < vc.taille(); ++i) {
		os << vc[i];
	}

	return os;
}

}  /* namespace dls */
