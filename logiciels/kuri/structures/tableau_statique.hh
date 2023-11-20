/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <cstdint>

namespace kuri {

/* Type pour passer des tableaux aux métaprogrammes. Ce type n'a pas de destructeurs, ou de
 * constructeurs, afin de ne pas tenter de libérer ou corrompre la mémoire. */
template <typename T>
struct tableau_statique {
  private:
    T *pointeur = nullptr;
    int64_t taille_ = 0;
    /* La capacité n'est que pour s'assurer que le tableau a la même taille que dans le langage. */
    int64_t capacite = 0;

  public:
    tableau_statique() = default;

    tableau_statique(T *ptr, int64_t nombre_elements) : pointeur(ptr), taille_(nombre_elements)
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
