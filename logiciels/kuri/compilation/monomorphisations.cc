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

#include "monomorphisations.hh"

#include "parsage/identifiant.hh"

#include "typage.hh"

std::ostream &operator<<(std::ostream &os, const ItemMonomorphisation &item)
{
    os << item.ident->nom << " " << chaine_type(item.type);
    if (!item.est_type) {
        os << " " << item.valeur;
    }
    return os;
}

NoeudExpression *BaseMonorphisations::trouve_monomorphisation_impl(const tableau_items &items) const
{
    auto monomorphisations_ = monomorphisations.verrou_lecture();

    POUR (*monomorphisations_) {
        if (it.premier.taille() != items.taille()) {
            continue;
        }

        auto trouve = true;

        for (auto i = 0; i < items.taille(); ++i) {
            if (it.premier[i] != items[i]) {
                trouve = false;
                break;
            }
        }

        if (!trouve) {
            continue;
        }

        return it.second;
    }

    return nullptr;
}

void BaseMonorphisations::ajoute(const tableau_items &items, NoeudExpression *noeud)
{
    monomorphisations->ajoute({items, noeud});
}

long BaseMonorphisations::memoire_utilisee() const
{
    long memoire = 0;
    memoire += monomorphisations->taille() *
            (taille_de(NoeudExpression *) + taille_de(tableau_items));

    POUR (*monomorphisations.verrou_lecture()) {
        memoire += it.premier.taille() * (taille_de(ItemMonomorphisation));
    }

    return memoire;
}

int BaseMonorphisations::taille() const
{
    return monomorphisations->taille();
}

int BaseMonorphisations::nombre_items_max() const
{
    int n = 0;

    POUR (*monomorphisations.verrou_lecture()) {
        if (it.premier.taille() > n) {
            n = it.premier.taille();
        }
    }

    return n;
}

void BaseMonorphisations::imprime(std::ostream &os)
{
    auto monomorphisations_ = monomorphisations.verrou_lecture();
    if (monomorphisations_->taille() == 0) {
        os << "Il n'y a aucune monomorphisation connue !\n";
        return;
    }

    os << "Les monomophisations sont :\n";
    POUR (*monomorphisations_) {
        os << "-- :\n";

        for (auto i = 0; i < it.premier.taille(); ++i) {
            os << "  -- " << it.premier[i] << '\n';
        }
    }
}
