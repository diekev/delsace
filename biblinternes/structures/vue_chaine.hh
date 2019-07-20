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

#include <cstring>
#include <functional>  /* pour la déclaration de std::hash */

namespace dls {

struct vue_chaine {
private:
	char const *m_ptr = nullptr;
	long m_taille = 0;

public:
	template <unsigned long N>
	vue_chaine(char const (&c)[N])
		: m_ptr(&c[0])
		, m_taille(N)
	{}

	vue_chaine() = default;

	vue_chaine(char const *ptr);

	vue_chaine(char const *ptr, long taille);

	char const &operator[](long idx) const;

	long taille() const;

	bool est_vide() const;

	const char *begin() const;

	const char *end() const;
};

bool operator<(vue_chaine const &c1, vue_chaine const &c2);

bool operator>(vue_chaine const &c1, vue_chaine const &c2);

bool operator==(vue_chaine const &vc1, vue_chaine const &vc2);

bool operator==(vue_chaine const &vc1, char const *vc2);

bool operator==(char const *vc1, vue_chaine const &vc2);

bool operator!=(vue_chaine const &vc1, vue_chaine const &vc2);

bool operator!=(vue_chaine const &vc1, char const *vc2);

bool operator!=(char const *vc1, vue_chaine const &vc2);

std::ostream &operator<<(std::ostream &os, vue_chaine const &vc);

}  /* namespace dls */

namespace std {

template <>
struct hash<dls::vue_chaine> {
	std::size_t operator()(dls::vue_chaine const &chn) const
	{
		auto h = std::hash<std::string>{};
		return h(std::string(&chn[0], static_cast<size_t>(chn.taille())));
	}
};

}  /* namespace std */
