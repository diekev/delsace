/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <iosfwd>

/**
 * Simple structure pour compter et imprimer le nombre de cas passant un prédicat.
 */
struct CompteuseCas {
  private:
    int cas_totaux = 0;
    int cas_vrai = 0;

  public:
    void ajoute_cas(bool prédicat);

    /* Imprime le résultat sour la forme "cas_vrai / cas_totaux (cas_totaux - cas_vrai)" */
    void imprime(std::ostream &os);
};
