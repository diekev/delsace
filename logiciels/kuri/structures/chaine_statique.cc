/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "chaine_statique.hh"

#include <cstring>
#include <iostream>

#include "utilitaires/unicode.hh"

namespace kuri {

int64_t chaine_statique::décalage_pour_caractère(int64_t i) const
{
    auto résultat = int64_t(0);
    auto n = unicode::nombre_octets(&pointeur_[i]);

    while (n == 0 && i < taille()) {
        ++résultat;
        ++i;
        n = unicode::nombre_octets(&pointeur_[i]);
    }

    résultat += n;

    return résultat;
}

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
