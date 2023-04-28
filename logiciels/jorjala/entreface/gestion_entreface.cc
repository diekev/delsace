/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

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
