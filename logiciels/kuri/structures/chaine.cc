/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "chaine.hh"

#include <cstring>
#include <iostream>
#include <utility>

#include "tableau.hh"

namespace kuri {

bool operator==(const chaine &chn1, const chaine &chn2)
{
    if (chn1.taille() != chn2.taille()) {
        return false;
    }

    if (chn1.taille() == 0) {
        return true;
    }

    for (auto i = 0; i < chn1.taille(); ++i) {
        if (chn1[i] != chn2[i]) {
            return false;
        }
    }

    return true;
}

bool operator==(chaine const &chn1, chaine_statique const &chn2)
{
    return kuri::chaine_statique(chn1) == chn2;
}

bool operator==(chaine_statique const &chn1, chaine const &chn2)
{
    return kuri::chaine_statique(chn2) == chn1;
}

bool operator==(chaine const &chn1, const char *chn2)
{
    return kuri::chaine_statique(chn2) == chn1;
}

bool operator==(const char *chn1, chaine const &chn2)
{
    return kuri::chaine_statique(chn1) == chn2;
}

bool operator!=(const chaine &chn1, const chaine &chn2)
{
    return !(chn1 == chn2);
}

bool operator!=(chaine const &chn1, chaine_statique const &chn2)
{
    return !(chn1 == chn2);
}

bool operator!=(chaine_statique const &chn1, chaine const &chn2)
{
    return !(chn1 == chn2);
}

bool operator!=(chaine const &chn1, const char *chn2)
{
    return !(chn1 == chn2);
}

bool operator!=(const char *chn1, chaine const &chn2)
{
    return !(chn1 == chn2);
}

std::ostream &operator<<(std::ostream &os, const chaine &chn)
{
    POUR (chn) {
        os << it;
    }

    return os;
}

int64_t distance_levenshtein(chaine_statique const &chn1, chaine_statique const &chn2)
{
    auto const m = chn1.taille();
    auto const n = chn2.taille();

    if (m == 0) {
        return n;
    }

    if (n == 0) {
        return m;
    }

    auto couts = tableau<int64_t>(n + 1);

    for (auto k = int64_t(0); k <= n; k++) {
        couts[k] = k;
    }

    for (auto i = int64_t(0); i < chn1.taille(); ++i) {
        couts[0] = i + 1;
        auto coin = i;

        for (auto j = 0; j < chn2.taille(); ++j) {
            auto enhaut = couts[j + 1];

            if (chn1.pointeur()[i] == chn2.pointeur()[j]) {
                couts[j + 1] = coin;
            }
            else {
                auto t = enhaut < coin ? enhaut : coin;
                couts[j + 1] = (couts[j] < t ? couts[j] : t) + 1;
            }

            coin = enhaut;
        }
    }

    return couts[n];
}

static bool est_espace_blanche(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

kuri::chaine supprime_espaces_blanches_autour(kuri::chaine_statique chaine)
{
    auto début = int64_t(0);
    auto fin = chaine.taille();

    for (auto i = 0; i < chaine.taille(); i++) {
        if (!est_espace_blanche(chaine.pointeur()[i])) {
            break;
        }
        début += 1;
    }

    for (auto i = chaine.taille() - 1; i >= 0; i--) {
        if (!est_espace_blanche(chaine.pointeur()[i])) {
            break;
        }
        fin -= 1;
    }

    return chaine.sous_chaine(début, fin);
}

}  // namespace kuri
