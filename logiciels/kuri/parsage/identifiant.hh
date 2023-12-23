/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"

struct IdentifiantCode {
    kuri::chaine_statique nom{};
    kuri::chaine_statique nom_broye{};
};

struct TableIdentifiant {
  private:
    kuri::table_hachage<kuri::chaine_statique, IdentifiantCode *> table{"IdentifiantCode"};
    tableau_page<IdentifiantCode, 1024> identifiants{};

    Enchaineuse enchaineuse{};

  public:
    TableIdentifiant();

    IdentifiantCode *identifiant_pour_chaine(kuri::chaine_statique nom);

    IdentifiantCode *identifiant_pour_nouvelle_chaine(kuri::chaine const &nom);

    int64_t taille() const;

    int64_t memoire_utilisee() const;

  private:
    IdentifiantCode *ajoute_identifiant(kuri::chaine_statique nom);
};

namespace ID {
#define ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(x, y) extern IdentifiantCode *x;
#include "identifiant.def"
#undef ENUMERE_IDENTIFIANT_COMMUN_SIMPLE
}  // namespace ID

void initialise_identifiants(TableIdentifiant &table);
