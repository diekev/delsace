/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "metaprogramme.hh"

#include "statistiques/statistiques.hh"

#include "typage.hh"

int DonneesConstantesExecutions::ajoute_globale(Type *type,
                                                IdentifiantCode *ident,
                                                void *adresse_pour_execution)
{
    auto globale = Globale{};
    globale.type = type;
    globale.ident = ident;
    globale.adresse = donnees_globales.taille();
    globale.adresse_pour_execution = adresse_pour_execution;

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
