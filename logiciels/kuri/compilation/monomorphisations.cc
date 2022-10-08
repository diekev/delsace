/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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

NoeudExpression *Monomorphisations::trouve_monomorphisation(const tableau_items &items) const
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

void Monomorphisations::ajoute(const tableau_items &items, NoeudExpression *noeud)
{
    monomorphisations->ajoute({items, noeud});
}

long Monomorphisations::memoire_utilisee() const
{
    long memoire = 0;
    memoire += monomorphisations->taille() *
            (taille_de(NoeudExpression *) + taille_de(tableau_items));

    POUR (*monomorphisations.verrou_lecture()) {
        memoire += it.premier.taille() * (taille_de(ItemMonomorphisation));
    }

    return memoire;
}

int Monomorphisations::taille() const
{
    return monomorphisations->taille();
}

int Monomorphisations::nombre_items_max() const
{
    int n = 0;

    POUR (*monomorphisations.verrou_lecture()) {
        if (it.premier.taille() > n) {
            n = it.premier.taille();
        }
    }

    return n;
}

void Monomorphisations::imprime(std::ostream &os)
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
