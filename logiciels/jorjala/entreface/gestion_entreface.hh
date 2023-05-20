/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/chaine.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QComboBox>
#pragma GCC diagnostic pop

namespace JJL {
class Jorjala;
}

class BaseEditrice;

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice);

enum {
    FICHIER_SAUVEGARDE,
    FICHIER_OUVERTURE,
};

dls::chaine affiche_dialogue(int type, dls::chaine const &filtre);

void appele_commande(JJL::Jorjala &jorjala,
                     dls::chaine const &nom_commande,
                     dls::chaine const &métadonnée);

struct DonnéesItemComboxBox {
    dls::chaine nom{};
    dls::chaine valeur{};
};

template <typename T, typename R>
void ajourne_combo_box(QComboBox *box,
                       dls::chaine const &valeur_courante,
                       T &tableau,
                       R &&donne_données_item)
{
    auto const les_signaux_sont_bloqués = box->blockSignals(true);
    box->clear();

    auto index_courant = 0;
    auto index_valeur_courante = 0;

    for (auto élément : tableau) {
        auto données_item = donne_données_item(élément);

        box->addItem(données_item.nom.c_str(), QVariant(données_item.valeur.c_str()));

        if (données_item.valeur == valeur_courante) {
            index_valeur_courante = index_courant;
        }

        index_courant++;
    }

    box->setCurrentIndex(index_valeur_courante);
    box->blockSignals(les_signaux_sont_bloqués);
}
