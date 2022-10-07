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

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/tuples.hh"

#include "arbre_syntaxique/expression.hh"

#include "structures/tableau.hh"

struct IdentifiantCode;
struct Type;

struct ItemMonomorphisation {
    const IdentifiantCode *ident = nullptr;
    const Type *type = nullptr;
    ValeurExpression valeur{};
    bool est_type = false;

    bool operator==(ItemMonomorphisation const &autre) const
    {
        if (ident != autre.ident) {
            return false;
        }

        if (type != autre.type) {
            return false;
        }

        if (est_type != autre.est_type) {
            return false;
        }

        if (!est_type) {
            if (valeur != autre.valeur) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(ItemMonomorphisation const &autre) const
    {
        return !(*this == autre);
    }
};

std::ostream &operator<<(std::ostream &os, const ItemMonomorphisation &item);

struct BaseMonorphisations {
  protected:
    template <typename T>
    using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

    using tableau_items = kuri::tableau<ItemMonomorphisation, int>;
    tableau_synchrone<dls::paire<tableau_items, NoeudExpression *>> monomorphisations{};

    NoeudExpression *trouve_monomorphisation_impl(tableau_items const &items) const;

  public:
    void ajoute(tableau_items const &items, NoeudExpression *noeud);

    long memoire_utilisee() const;

    int taille() const;

    int nombre_items_max() const;

    void imprime(std::ostream &os);
};

template <typename TypeNoeud>
struct Monomorphisations : public BaseMonorphisations {
    TypeNoeud *trouve_monomorphisation(tableau_items const &items) const
    {
        return static_cast<TypeNoeud *>(trouve_monomorphisation_impl(items));
    }
};
