/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "metaprogramme.hh"

#include "statistiques/statistiques.hh"

#include "typage.hh"

int DonneesConstantesExecutions::ajoute_globale(Type *type,
                                                IdentifiantCode *ident,
                                                const Type *pour_info_type)
{
    auto globale = Globale{};
    globale.type = type;
    globale.ident = ident;
    globale.adresse = donnees_globales.taille();

    if (pour_info_type) {
        globale.adresse_pour_execution = pour_info_type->info_type;
    }
    else {
        globale.adresse_pour_execution = nullptr;
    }

    donnees_globales.redimensionne(donnees_globales.taille() +
                                   static_cast<int>(type->taille_octet));

    auto ptr = globales.taille();

    globales.ajoute(globale);

    return ptr;
}

void DonneesConstantesExecutions::rassemble_statistiques(Statistiques &stats) const
{
    auto memoire_mv = 0l;
    memoire_mv += globales.taille_memoire();
    memoire_mv += donnees_constantes.taille_memoire();
    memoire_mv += donnees_globales.taille_memoire();
    memoire_mv += patchs_donnees_constantes.taille_memoire();

    stats.memoire_mv += memoire_mv;
}
