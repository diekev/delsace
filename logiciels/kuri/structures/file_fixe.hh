/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

namespace kuri {

template <typename T, uint64_t N>
struct file_fixe {
  private:
    T valeurs[N];
    long curseur = 0;
    long tete = 0;

  public:
    void enfile(T valeur)
    {
        valeurs[curseur] = valeur;
        curseur += 1;
    }

    T defile()
    {
        auto valeur = valeurs[tete];
        tete += 1;
        return valeur;
    }

    bool est_vide() const
    {
        return tete == curseur;
    }

    long taille() const
    {
        return curseur - tete;
    }

    void efface()
    {
        curseur = 0;
        tete = 0;
    }
};

} /* namespace kuri */
