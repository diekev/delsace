/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "infos_types.hh"

#include "utilitaires/log.hh"

int64_t AllocatriceInfosType::memoire_utilisee() const
{
    auto memoire = int64_t(0);
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
    memoire += infos_types_variadiques.memoire_utilisee();

#define ENUME_TYPES_TRANCHES_INFO_TYPE_EX(type__, nom__)                                          \
    memoire += tranches_##nom__.taille_memoire();                                                 \
    POUR (tranches_##nom__) {                                                                     \
        memoire += it.taille() * taille_de(type__);                                               \
    }

    ENUME_TYPES_TRANCHES_INFO_TYPE(ENUME_TYPES_TRANCHES_INFO_TYPE_EX)

#undef ENUME_TYPES_TRANCHES_INFO_TYPE_EX

    return memoire;
}
