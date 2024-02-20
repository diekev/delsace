/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <algorithm>

#include "structures/tableau.hh"

namespace kuri {

/* ------------------------------------------------------------------------- */
/** \name Partition de tableau.
 * \{ */

/**
 * Type de sortie de #partition_stable. Stocke les deux partitions.
 */
template <typename Type>
struct partition_tableau {
    /* Partition pour tous les éléments dont le prédicat s'applique. */
    tableau_statique<Type> vrai{};
    /* Partition pour tous les éléments dont le prédicat ne s'applique pas. */
    tableau_statique<Type> faux{};
};

template <typename Type, typename Prédicat>
partition_tableau<Type> partition_stable(tableau_statique<Type> éléments, Prédicat prédicat)
{
    auto iter = std::stable_partition(éléments.begin(), éléments.end(), std::move(prédicat));
    auto taille = std::distance(iter, éléments.end());
    auto tableau_vrai = tableau_statique(éléments.begin(), éléments.taille() - taille);
    auto tableau_faux = tableau_statique(iter, taille);
    return {tableau_vrai, tableau_faux};
}

template <typename Type, typename Prédicat>
partition_tableau<Type> partition_stable(tableau<Type> &éléments, Prédicat prédicat)
{
    auto iter = std::stable_partition(éléments.begin(), éléments.end(), std::move(prédicat));
    auto taille = std::distance(iter, éléments.end());
    auto tableau_vrai = tableau_statique(éléments.begin(), éléments.taille() - taille);
    auto tableau_faux = tableau_statique(iter, taille);
    return {tableau_vrai, tableau_faux};
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Triage de tableaux.
 * \{ */

template <typename Type, typename Prédicat>
void tri_stable(tableau_statique<Type> éléments, Prédicat prédicat)
{
    std::stable_sort(éléments.begin(), éléments.end(), std::move(prédicat));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Triage de tableaux.
 * \{ */

template <typename Type, typename Prédicat>
void tri_rapide(tableau_statique<Type> éléments, Prédicat prédicat)
{
    std::sort(éléments.begin(), éléments.end(), std::move(prédicat));
}

/** \} */

}  // namespace kuri
