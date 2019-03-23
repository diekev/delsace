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

#include "logeuse_memoire.hh"

#include <iostream>
#include <sstream>

namespace memoire {

logeuse_memoire logeuse_memoire::m_instance = logeuse_memoire{};

logeuse_memoire::~logeuse_memoire()
{
	if (memoire_allouee != 0) {
		std::cerr << "Fuite de mémoire ou désynchronisation : "
				  << formate_taille(memoire_allouee) << '\n';
	}
}

logeuse_memoire &logeuse_memoire::instance()
{
	return m_instance;
}

size_t allouee()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.memoire_allouee;
}

size_t consommee()
{
	auto &logeuse = logeuse_memoire::instance();
	return logeuse.memoire_consommee;
}

std::string formate_taille(size_t octets)
{
	std::stringstream ss;

	if (octets < 1024) {
		ss << octets << " o";
	}
	else if (octets < (1024 * 1024)) {
		ss << octets / (1024) << " Ko";
	}
	else if (octets < (1024 * 1024 * 1024)) {
		ss << octets / (1024 * 1024) << " Mo";
	}
	else {
		ss << octets / (1024 * 1024 * 1024) << " Go";
	}

	return ss.str();
}

}  /* namespace memoire */
