/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "tableau.hh"

namespace kuri {

template <typename T>
struct pile {
    using type_valeur = T;
    using type_reference = T &;
    using type_reference_const = T const &;
    using type_taille = int64_t;

  private:
    tableau<type_valeur> m_pile{};

  public:
    pile() = default;

    void empile(T &&valeur)
    {
        m_pile.ajoute(valeur);
    }

    void empile(type_reference_const valeur)
    {
        m_pile.ajoute(valeur);
    }

    type_valeur depile()
    {
        auto t = this->haut();
        m_pile.supprime_dernier();
        return t;
    }

    type_reference haut()
    {
        return m_pile.dernier_élément();
    }

    type_reference_const haut() const
    {
        return m_pile.dernier_élément();
    }

    bool est_vide() const
    {
        return m_pile.est_vide();
    }

    void efface()
    {
        m_pile.efface();
    }

    type_taille taille() const
    {
        return m_pile.taille();
    }
};

template <typename T, uint64_t N>
struct pile_fixe {
    using type_valeur = T;
    using type_reference = T &;
    using type_reference_const = T const &;
    using type_taille = int64_t;

  private:
    type_valeur m_pile[N];
    type_taille m_index = -1;

  public:
    pile_fixe() = default;

    void empile(type_reference_const valeur)
    {
        if (m_index + 1 < N) {
            m_pile[++m_index] = valeur;
        }
    }

    type_valeur depile()
    {
        auto t = haut();
        --m_index;
        return t;
    }

    type_reference haut()
    {
        return m_pile[m_index];
    }

    type_reference_const haut() const
    {
        return m_pile[m_index];
    }

    bool est_vide() const
    {
        return m_index < 0;
    }

    bool est_pleine() const
    {
        return m_index == N;
    }

    type_taille taille() const
    {
        return m_index + 1;
    }
};

} /* namespace kuri */
