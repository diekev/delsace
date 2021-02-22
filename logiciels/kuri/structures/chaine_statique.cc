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

#include "chaine_statique.hh"

#include <cstring>
#include <iostream>

namespace kuri {

static int compare_chaine(chaine_statique const &chn1, chaine_statique const &chn2)
{
	auto p1 = chn1.pointeur();
	auto p2 = chn2.pointeur();

	auto t1 = chn1.taille();
	auto t2 = chn2.taille();

#if 0
	for (auto i = 0; i < t1; ++i) {
		if (i == t2) {
			return 0;
		}

		if (static_cast<unsigned char>(*p2) > static_cast<unsigned char>(*p1)) {
			return -1;
		}

		if (static_cast<unsigned char>(*p1) > static_cast<unsigned char>(*p2)) {
			return 1;
		}

		++p1;
		++p2;
	}

	if (t1 < t2) {
		return -1;
	}

	return 0;
#else
	auto taille = std::max(t1, t2);
	return strncmp(p1, p2, static_cast<size_t>(taille));
#endif
}

bool operator<(const chaine_statique &c1, const chaine_statique &c2)
{
	return compare_chaine(c1, c2) < 0;
}

bool operator>(const chaine_statique &c1, const chaine_statique &c2)
{
	return compare_chaine(c1, c2) > 0;
}

bool operator==(chaine_statique const &vc1, chaine_statique const &vc2)
{
	return compare_chaine(vc1, vc2) == 0;
}

bool operator==(chaine_statique const &vc1, char const *vc2)
{
	return vc1 == chaine_statique(vc2);
}

bool operator==(char const *vc1, chaine_statique const &vc2)
{
	return vc2 == chaine_statique(vc1);
}

bool operator!=(chaine_statique const &vc1, chaine_statique const &vc2)
{
	return compare_chaine(vc1, vc2) != 0;
}

bool operator!=(chaine_statique const &vc1, char const *vc2)
{
	return !(vc1 == vc2);
}

bool operator!=(char const *vc1, chaine_statique const &vc2)
{
	return !(vc1 == vc2);
}

std::ostream &operator<<(std::ostream &os, const chaine_statique &vc)
{
	for (auto i = 0; i < vc.taille(); ++i) {
		os << vc.pointeur()[i];
	}

	return os;
}

}
