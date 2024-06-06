/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include <QString>
#include <optional>

namespace danjo {

class DialoguesChemins {
  public:
    virtual ~DialoguesChemins() = default;

    virtual QString donne_chemin_pour_ouverture(QString const &chemin_existant,
                                                QString const &caption,
                                                QString const &dossier,
                                                QString const &filtres) = 0;
    virtual QString donne_chemin_pour_écriture(QString const &chemin_existant,
                                               QString const &caption,
                                               QString const &dossier,
                                               QString const &filtres) = 0;
    virtual QString donne_chemin_pour_dossier(QString const &chemin_existant,
                                              QString const &caption,
                                              QString const &dossier) = 0;
};

DialoguesChemins &donne_dialogues_chemins();

void définis_dialogues_chemins(DialoguesChemins &fournisseuse);

}  // namespace danjo
