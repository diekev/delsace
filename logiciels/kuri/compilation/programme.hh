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

#include "biblinternes/structures/ensemble.hh"

#include "structures/tableau.hh"

struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct Type;

/* Représentation d'un programme. Ceci peut être le programme final tel que généré par la
 * compilation ou bien un métaprogramme. */
struct Programme {
  protected:
    kuri::tableau<NoeudDeclarationEnteteFonction *> fonctions{};
    dls::ensemble<NoeudDeclarationEnteteFonction *> fonctions_utilisees{};

    kuri::tableau<NoeudDeclarationVariable *> globales{};
    dls::ensemble<NoeudDeclarationVariable *> globales_utilisees{};

    kuri::tableau<Type *> types{};
    dls::ensemble<Type *> types_utilises{};

  public:
    void ajoute_fonction(NoeudDeclarationEnteteFonction *fonction);

    void ajoute_globale(NoeudDeclarationVariable *globale);

    void ajoute_type(Type *type);

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs types validés. */
    bool typages_termines() const;

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs RI générées. */
    bool ri_generees() const;
};
