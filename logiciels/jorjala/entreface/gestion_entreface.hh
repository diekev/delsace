/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

namespace dls {
class chaine;
}

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
