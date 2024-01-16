/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <queue>

#include "tableau.hh"

namespace kuri {

template <typename T>
struct file {
    using type_valeur = T;
    using type_reference = T &;
    using type_reference_const = T const &;
    using type_taille = int64_t;

  private:
    tableau<type_valeur> m_file{};

    using iteratrice = typename tableau<type_valeur>::iteratrice;
    using iteratrice_const = typename tableau<type_valeur>::iteratrice_const;

  public:
    file() = default;

    bool est_vide() const
    {
        return m_file.est_vide();
    }

    type_taille taille() const
    {
        return m_file.taille();
    }

    type_reference front()
    {
        return m_file.premier_élément();
    }

    type_reference_const front() const
    {
        return m_file.premier_élément();
    }

    void enfile(type_reference_const valeur)
    {
        m_file.ajoute(valeur);
    }

    void enfile(tableau<type_valeur> const &valeurs)
    {
        for (auto const &valeur : valeurs) {
            m_file.ajoute(valeur);
        }
    }

    void efface()
    {
        m_file.efface();
    }

    template <typename Predicat>
    void efface_si(Predicat &&predicat)
    {
        auto fin = std::remove_if(m_file.debut(), m_file.fin(), predicat);
        m_file.efface(fin, m_file.fin());
    }

    type_valeur defile()
    {
        auto t = front();
        m_file.supprime_premier();
        return t;
    }

    tableau<type_valeur> defile(int64_t compte)
    {
        auto ret = tableau<type_valeur>(compte);

        for (auto i = 0; i < compte; ++i) {
            ret.ajoute(defile());
        }

        return ret;
    }

    iteratrice begin()
    {
        return m_file.debut();
    }

    iteratrice end()
    {
        return m_file.fin();
    }

    iteratrice_const begin() const
    {
        return m_file.debut();
    }

    iteratrice_const end() const
    {
        return m_file.fin();
    }
};

} /* namespace kuri */
