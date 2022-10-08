/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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

struct Monomorphisations {
  protected:
    template <typename T>
    using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

    using tableau_items = kuri::tableau<ItemMonomorphisation, int>;
    tableau_synchrone<dls::paire<tableau_items, NoeudExpression *>> monomorphisations{};

  public:
    void ajoute(tableau_items const &items, NoeudExpression *noeud);

    NoeudExpression *trouve_monomorphisation(tableau_items const &items) const;

    long memoire_utilisee() const;

    int taille() const;

    int nombre_items_max() const;

    void imprime(std::ostream &os);
};
