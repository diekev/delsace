/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#include "divers.hh"

int nombre_de_chiffres(int64_t nombre)
{
    if (nombre == 0) {
        return 1;
    }

    auto compte = 0;

    while (nombre > 0) {
        nombre /= 10;
        compte += 1;
    }

    return compte;
}
