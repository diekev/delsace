/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "structures/enchaineuse.hh"

struct GeranteChaine {
  private:
    Enchaineuse enchaineuse{};

  public:
    int64_t ajoute_chaine(kuri::chaine_statique chaine);
    int64_t ajoute_chaine(kuri::chaine const &chaine);

    kuri::chaine_statique chaine_pour_adresse(int64_t adresse) const;

    int64_t mémoire_utilisée() const;
};
