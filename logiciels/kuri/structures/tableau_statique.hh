/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

namespace kuri {

/* Type pour passer des tableaux aux métaprogrammes. Ce type n'a pas de destructeurs, ou de
 * constructeurs, afin de ne pas tenter de libérer ou corrompre la mémoire. */
template <typename T>
struct tableau_statique {
  private:
    T const *pointeur = nullptr;
    long taille_ = 0;
    /* La capacité n'est que pour s'assurer que le tableau a la même taille que dans le langage. */
    long capacite = 0;

    tableau_statique() = default;

  public:
    tableau_statique(T *ptr, long nombre_elements) : pointeur(ptr), taille_(nombre_elements)
    {
    }

    long taille() const
    {
        return taille_;
    }

    T const *begin() const
    {
        return pointeur;
    }

    T const *end() const
    {
        return pointeur + taille();
    }

    T const &operator[](long i)
    {
        return pointeur[i];
    }
};
}  // namespace kuri
