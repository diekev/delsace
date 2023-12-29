/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <cstdint>

namespace kuri {

/**
 * Classe représentant le type tranche du langage.
 * Elle doit avoir la même disposition mémoire que celui-ci afin de pouvoir
 * être passée aux et retournée des métaprogrammes.
 */
template <typename T>
class tranche {
    T *pointeur = nullptr;
    int64_t taille_ = 0;

  public:
    tranche() = default;

    tranche(T *ptr, int64_t nombre_elements) : pointeur(ptr), taille_(nombre_elements)
    {
    }

    int64_t taille() const
    {
        return taille_;
    }

    T *begin()
    {
        return pointeur;
    }

    T *end()
    {
        return pointeur + taille();
    }

    T const *begin() const
    {
        return pointeur;
    }

    T const *end() const
    {
        return pointeur + taille();
    }

    T const &operator[](int64_t i) const
    {
        return pointeur[i];
    }
};

}  // namespace kuri
