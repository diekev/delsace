/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "chaine_statique.hh"

#include <cstring>
#include <iostream>

namespace kuri {

#if 0
static int compare_chaine(chaine_statique const &chn1, chaine_statique const &chn2)
{
	auto p1 = chn1.pointeur();
	auto p2 = chn2.pointeur();

	auto t1 = chn1.taille();
	auto t2 = chn2.taille();

#    if 0
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
#    else
	auto taille = std::max(t1, t2);
	return strncmp(p1, p2, static_cast<size_t>(taille));
#    endif
}
#endif

bool operator<(const chaine_statique &c1, const chaine_statique &c2)
{
    return std::string_view{c1.pointeur(), static_cast<size_t>(c1.taille())} <
           std::string_view{c2.pointeur(), static_cast<size_t>(c2.taille())};
}

bool operator>(const chaine_statique &c1, const chaine_statique &c2)
{
    return std::string_view{c1.pointeur(), static_cast<size_t>(c1.taille())} >
           std::string_view{c2.pointeur(), static_cast<size_t>(c2.taille())};
}

bool operator==(chaine_statique const &c1, chaine_statique const &c2)
{
    return std::string_view{c1.pointeur(), static_cast<size_t>(c1.taille())} ==
           std::string_view{c2.pointeur(), static_cast<size_t>(c2.taille())};
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
    return !(vc1 == vc2);
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

}  // namespace kuri
