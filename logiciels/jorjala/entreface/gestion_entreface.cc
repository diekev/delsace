/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#pragma GCC diagnostic pop

#include "base_editrice.h"

#include "coeur/jorjala.hh"

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice)
{
    auto données = accède_données_programme(jorjala);

    if (données->editrice_active) {
        données->editrice_active->actif(false);
    }

    données->editrice_active = editrice;
}

dls::chaine affiche_dialogue(int type, dls::chaine const &filtre)
{
    auto parent = static_cast<QWidget *>(nullptr);
    auto caption = "";
    auto dir = "";

    /* À FAIRE : sort ça de la classe. */
    if (type == FICHIER_OUVERTURE) {
        auto const chemin = QFileDialog::getOpenFileName(
                    parent,
                    caption,
                    dir,
                    filtre.c_str());
        return chemin.toStdString();
    }

    if (type == FICHIER_SAUVEGARDE) {
        auto const chemin = QFileDialog::getSaveFileName(
                    parent,
                    caption,
                    dir,
                    filtre.c_str());
        return chemin.toStdString();
    }

    return "";
}
