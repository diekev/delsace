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

#pragma once

#include "biblinternes/structures/dico.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

struct IdentifiantCode;
struct Statistiques;

struct GestionnaireBibliotheques {
    struct BibliothequePartagee {
        dls::systeme_fichier::shared_library bib{};
        kuri::chaine chemin{};
    };

    using type_fonction = void (*)();

  private:
    kuri::tableau<BibliothequePartagee> bibliotheques{};
    dls::dico<IdentifiantCode *, type_fonction> symboles_et_fonctions{};

  public:
    void ajoute_bibliotheque(kuri::chaine const &chemin);
    void ajoute_fonction_pour_symbole(IdentifiantCode *symbole, type_fonction fonction);
    type_fonction fonction_pour_symbole(IdentifiantCode *symbole);

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;
};
