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

#pragma once

#include <sstream>

#include "biblinternes/memoire/logeuse_gardee.hh"

namespace dls {

struct flux_chaine {
	using type_flux = std::basic_stringstream<char, std::char_traits<char>, memoire::logeuse_guardee<char>>;
	using type_chaine = std::basic_string<char, std::char_traits<char>, memoire::logeuse_guardee<char>>;

private:

	type_flux m_flux{};

public:
	flux_chaine() = default;
	~flux_chaine() = default;

	flux_chaine(type_chaine const &c);

	type_chaine chn() const;

	void chn(type_chaine const &c);

	type_flux &flux();
};

template <typename T>
flux_chaine &operator<<(flux_chaine &flux, T const &v)
{
	flux.flux() << v;
	return flux;
}

template <typename T>
flux_chaine &operator>>(flux_chaine &flux, T &v)
{
	flux.flux() >> v;
	return flux;
}

}  /* namespace dls */
