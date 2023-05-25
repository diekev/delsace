/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "infos_types.hh"

int64_t AllocatriceInfosType::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += infos_types.memoire_utilisee();
    memoire += infos_types_entiers.memoire_utilisee();
    memoire += infos_types_enums.memoire_utilisee();
    memoire += infos_types_fonctions.memoire_utilisee();
    memoire += infos_types_membres_structures.memoire_utilisee();
    memoire += infos_types_pointeurs.memoire_utilisee();
    memoire += infos_types_structures.memoire_utilisee();
    memoire += infos_types_tableaux.memoire_utilisee();
    memoire += infos_types_unions.memoire_utilisee();
    memoire += infos_types_opaques.memoire_utilisee();
    return memoire;
}
