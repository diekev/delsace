/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "dialogues_chemins.hh"

#include <QFileDialog>

namespace danjo {

class DialoguesCheminsDefaut : public DialoguesChemins {
  public:
    QString donne_chemin_pour_ouverture(QString const &chemin_existant,
                                        QString const &caption,
                                        QString const &dossier,
                                        QString const &filtres) override
    {
        return QFileDialog::getOpenFileName(nullptr, caption, dossier, filtres);
    }

    QString donne_chemin_pour_écriture(QString const &chemin_existant,
                                       QString const &caption,
                                       QString const &dossier,
                                       QString const &filtres) override
    {
        return QFileDialog::getSaveFileName(nullptr, caption, dossier, filtres);
    }

    QString donne_chemin_pour_dossier(QString const &chemin_existant,
                                      QString const &caption,
                                      QString const &dossier) override
    {
        return QFileDialog::getExistingDirectory(nullptr, caption, dossier);
    }
};

static DialoguesCheminsDefaut __dialogues_defaut = {};
static DialoguesChemins *__dialogues = &__dialogues_defaut;

DialoguesChemins &donne_dialogues_chemins()
{
    return *__dialogues;
}

void définis_dialogues_chemins(DialoguesChemins &dialogues)
{
    __dialogues = &dialogues;
}

}  // namespace danjo
