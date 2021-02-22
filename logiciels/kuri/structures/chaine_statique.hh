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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cstring>     /* pour la déclaration de std::strlen */
#include <functional>  /* pour la déclaration de std::hash */

#include "biblinternes/structures/vue_chaine_compacte.hh"

namespace kuri {

/* ATTENTION: cette structure doit avoir la même IBA que le type « chaine » du langage,
 * car nous l'utilisons dans les fonctions d'interface de la Compilatrice.
 */
struct chaine_statique {
private:
	const char *pointeur_ = nullptr;
	long taille_ = 0;

public:
	chaine_statique() = default;

	chaine_statique(chaine_statique const &) = default;
	chaine_statique &operator=(chaine_statique const&) = default;

	chaine_statique(dls::vue_chaine_compacte chn)
		: chaine_statique(chn.pointeur(), chn.taille())
	{}

	chaine_statique(const char *ptr, long taille)
		: pointeur_(ptr)
		, taille_(taille)
	{}

	chaine_statique(const char *ptr)
		: chaine_statique(ptr, static_cast<long>(std::strlen(ptr)))
	{}

	template <unsigned long N>
	chaine_statique(const char (&ptr)[N])
		: chaine_statique(ptr, static_cast<long>(N))
	{}

	const char *pointeur() const
	{
		return pointeur_;
	}

	long taille() const
	{
		return taille_;
	}
};

bool operator<(chaine_statique const &c1, chaine_statique const &c2);

bool operator>(chaine_statique const &c1, chaine_statique const &c2);

bool operator==(chaine_statique const &vc1, chaine_statique const &vc2);

bool operator==(chaine_statique const &vc1, char const *vc2);

bool operator==(char const *vc1, chaine_statique const &vc2);

bool operator!=(chaine_statique const &vc1, chaine_statique const &vc2);

bool operator!=(chaine_statique const &vc1, char const *vc2);

bool operator!=(char const *vc1, chaine_statique const &vc2);

std::ostream &operator<<(std::ostream &os, chaine_statique const &vc);

}

namespace std {

template <>
struct hash<kuri::chaine_statique> {
	std::size_t operator()(kuri::chaine_statique const &chn) const
	{
		auto h = std::hash<std::string>{};
		return h(std::string(chn.pointeur(), static_cast<size_t>(chn.taille())));
	}
};

}  /* namespace std */
