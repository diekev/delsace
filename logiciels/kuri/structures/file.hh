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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <queue>

#include "tableau.hh"

namespace kuri {

template <typename T>
struct file {
    using type_valeur = T;
    using type_reference = T &;
    using type_reference_const = T const &;
    using type_taille = long;

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
        return m_file.premiere();
    }

    type_reference_const front() const
    {
        return m_file.premiere();
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

    tableau<type_valeur> defile(long compte)
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
