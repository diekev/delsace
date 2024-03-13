/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "pile.hh"
#include "tableau.hh"

namespace kuri {

/** Structure représentant une pile de tableaux. Cette structure est utile pour créer des sortes de
 * tables de symboles, etc. Elle permet d'ajouter des éléments à des tableaux empilés les uns sur
 * les autres. Les éléments sont toujours ajoutés au tableau en tête de pile. Ce dernier peut-être
 * accédé par vue (tableau_statique), mais une fois dépilé, les données ne sont plus valides. */
template <typename T>
class pile_de_tableaux {
    kuri::tableau<T, int> m_données{};
    kuri::pile<int> m_décalage_par_tableau{};

  public:
    void empile_tableau()
    {
        m_décalage_par_tableau.empile(m_données.taille());
    }

    void dépile_tableau()
    {
        m_données.redimensionne(m_décalage_par_tableau.depile());
    }

    /** Retourne la taille totale de tous les tableaux empilés. */
    int taille_données() const
    {
        return m_données.taille();
    }

    /** Retourne la taille totale de tous les tableaux sauf celui en tête de pile. */
    int décalage_courant() const
    {
        return m_décalage_par_tableau.haut();
    }

    kuri::tableau_statique<T> donne_tableau_courant()
    {
        if (m_données.est_vide() || m_décalage_par_tableau.est_vide()) {
            return {nullptr, 0};
        }

        /* Vérifie si des choses furent ajoutées. */
        if (m_données.taille() == m_décalage_par_tableau.haut()) {
            return {nullptr, 0};
        }

        return {&m_données[m_décalage_par_tableau.haut()],
                m_données.taille() - m_décalage_par_tableau.haut()};
    }

    void ajoute_au_tableau_courant(T donnée)
    {
        m_données.ajoute(donnée);
    }
};

}  // namespace kuri
