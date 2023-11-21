/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "ensemble.hh"
#include "tableau.hh"

namespace kuri {

/**
 * La #rassembleuse sers à créer un tableau unique de valeur.
 * Contrairement à #ensemble, les valeurs insérées sont accessible dans l'ordre dans lequel elles
 * furent ajoutées.
 * Cette structure est utile pour aplatir un graphe, pour par exemple avoir un tableau où les
 * valeurs dépendues sont avant les valeurs dépendantes..
 */
template <typename T>
struct rassembleuse {
  private:
    tableau<T> éléments{};
    ensemble<T> éléments_vus{};

  public:
    bool possède(T const élément) const
    {
        return éléments_vus.possède(élément);
    }

    void réinitialise()
    {
        éléments_vus.efface();
        éléments.efface();
    }

    void insère(T const élément)
    {
        if (éléments_vus.possède(élément)) {
            return;
        }

        éléments_vus.insère(élément);
        éléments.ajoute(élément);
    }

    tableau_statique<T> donne_éléments() const
    {
        return éléments;
    }

    tableau<T> donne_copie_éléments() const
    {
        return éléments;
    }
};

}  // namespace kuri
