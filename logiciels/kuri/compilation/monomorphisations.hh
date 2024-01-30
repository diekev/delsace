/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/tuples.hh"

#include "arbre_syntaxique/expression.hh"

#include "structures/tableau.hh"

struct Enchaineuse;
struct IdentifiantCode;
struct NoeudDeclarationType;
using Type = NoeudDeclarationType;

namespace kuri {
struct chaine_statique;
}

enum class GenreItem : uint8_t {
    /* Peut-être Structure($I). */
    INDÉFINI,
    /* Correspond à ...: $T ou $T: type_de_données */
    TYPE_DE_DONNÉES,
    /* Correspond à, par exemple, $V: z32 */
    VALEUR,
};

kuri::chaine_statique chaine_pour_genre_item(GenreItem genre);

struct ItemMonomorphisation {
    const IdentifiantCode *ident = nullptr;
    const Type *type = nullptr;
    ValeurExpression valeur{};
    GenreItem genre = GenreItem::INDÉFINI;
    NoeudExpression *expression_type = nullptr;

    bool operator==(ItemMonomorphisation const &autre) const
    {
        if (ident != autre.ident) {
            return false;
        }

        if (type != autre.type) {
            return false;
        }

        if (genre != autre.genre) {
            return false;
        }

        if (genre == GenreItem::VALEUR) {
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
    using tableau_items = kuri::tableau<ItemMonomorphisation, int>;
    kuri::tableau_synchrone<dls::paire<tableau_items, NoeudExpression *>> monomorphisations{};

  public:
    void ajoute(tableau_items const &items, NoeudExpression *noeud);

    NoeudExpression *trouve_monomorphisation(
        kuri::tableau_statique<ItemMonomorphisation> items) const;

    int64_t mémoire_utilisée() const;

    int taille() const;

    int nombre_items_max() const;

    void imprime(std::ostream &os) const;
    void imprime(Enchaineuse &os, int indentations = 0) const;
};
