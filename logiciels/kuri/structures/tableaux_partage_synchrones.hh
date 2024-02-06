/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "tableau.hh"

namespace kuri {

/* ------------------------------------------------------------------------- */
/** \name Tableaux partagés entre threads synchrones.
 *
 * Cette structure stocke 2 tableaux pour partager des données entre fils d'exécution.
 * Un tableau est pour les producteurs, l'autre pour le consommateur. Lorsque le consommateur veut
 * les données produites, les tableaux doivent être permutés pour que le consommateur ne bloque pas
 * les producteurs.
 * \{ */

template <typename T>
struct tableaux_partage_synchrones {
  private:
    /* Données globales, partagées entre tous les producteurs. */
    tableau_synchrone<T> données_globales{};
    /* Données locales pour le consommateur. Ces données sont en fait les données globales au
     * moment de la permutation des deux listes, pour que le consommateur travaille sur les données
     * produites jusqu'alors, et que les producteurs travaillent sur un tableau différent. Ce
     * tableau doit être remis à zéro avant la permutation. */
    tableau<T, int> données_locales{};

  public:
    void ajoute_aux_données_globales(T valeur)
    {
        données_globales->ajoute(valeur);
    }

    void ajoute_aux_données_globales(tableau_statique<T> valeurs)
    {
        auto données_globales_ = données_globales.verrou_ecriture();
        POUR (valeurs) {
            données_globales_->ajoute(it);
        }
    }

    void efface_tout()
    {
        données_locales.efface();
        données_globales->efface();
    }

    bool possède_élément_dans_données_globales() const
    {
        return !données_globales->est_vide();
    }

    /* Permute les tableaux, à faire quand le consommateur veut traiter les données globales. */
    void permute_données_globales_et_locales()
    {
        /* Efface les anciennes données locales pour que l'ajout dans les données globales reparte
         * de zéro. */
        données_locales.efface();
        données_globales->permute(données_locales);
    }

    tableau_statique<T> donne_données_locales() const
    {
        return données_locales;
    }

    int64_t taille_mémoire() const
    {
        return données_globales->taille_mémoire() + données_locales.taille_mémoire();
    }
};

/** \} */

}  // namespace kuri
