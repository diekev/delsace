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

#include <functional>  /* pour la déclaration de std::hash */
#include <string_view>

#include "pointeur_marque.hh"

namespace dls {

struct vue_chaine_compacte {
private:
	pointeur_marque_haut<char const> m_ptr{};

public:
	template <uint64_t N>
	vue_chaine_compacte(char const (&c)[N])
		: m_ptr(&c[0], N)
	{}

	vue_chaine_compacte() = default;

	vue_chaine_compacte(char const *ptr);

	inline vue_chaine_compacte(char const *ptr, int64_t taille)
		: m_ptr(ptr, static_cast<int>(taille))
	{}

	inline char const &operator[](int64_t idx) const
	{
		return pointeur()[idx];
	}

	inline int64_t taille() const
	{
		return m_ptr.marque();
	}

	inline bool est_vide() const
	{
		return taille() == 0;
	}

	inline char const *pointeur() const
	{
		return m_ptr.pointeur();
	}

	inline const char *begin() const
	{
		return pointeur();
	}

	inline const char *end() const
	{
		return pointeur() + taille();
	}
};

bool operator<(vue_chaine_compacte const &c1, vue_chaine_compacte const &c2);

bool operator>(vue_chaine_compacte const &c1, vue_chaine_compacte const &c2);

bool operator==(vue_chaine_compacte const &vc1, vue_chaine_compacte const &vc2);

bool operator==(vue_chaine_compacte const &vc1, char const *vc2);

bool operator==(char const *vc1, vue_chaine_compacte const &vc2);

bool operator!=(vue_chaine_compacte const &vc1, vue_chaine_compacte const &vc2);

bool operator!=(vue_chaine_compacte const &vc1, char const *vc2);

bool operator!=(char const *vc1, vue_chaine_compacte const &vc2);

std::ostream &operator<<(std::ostream &os, vue_chaine_compacte const &vc);

}  /* namespace dls */

namespace std {

template <>
struct hash<dls::vue_chaine_compacte> {
	std::size_t operator()(dls::vue_chaine_compacte const &chn) const
	{
		auto h = std::hash<std::string_view>{};
		return h(std::string_view(&chn[0], static_cast<size_t>(chn.taille())));
	}
};

}  /* namespace std */
