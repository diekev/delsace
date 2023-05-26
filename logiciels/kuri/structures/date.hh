/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <cstdint>

struct TempsSysteme {
    int64_t seconde;
    int64_t nanoseconde;
};

TempsSysteme maintenant_systeme();

struct Date {
    int annee;
    int mois;
    int jour;
    int jour_semaine;

    int heure;
    int minute;
    int seconde;
};

bool est_annee_bissextile(int64_t annee);

inline int secondes_par_an()
{
    return 31556952;
}

Date hui_systeme();
