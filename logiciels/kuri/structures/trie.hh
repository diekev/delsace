/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include <variant>

#include "table_hachage.hh"
#include "tablet.hh"

namespace kuri {

template <typename TypeIdentifiant, typename TypeDonnées>
struct trie {
    static constexpr auto TAILLE_MAX_ENFANTS_TABLET = 16;
    struct Noeud;

    /**
     * Structure pour abstraire les listes d'enfants des noeuds.
     *
     * Par défaut nous utilisons un tablet, mais si nous avons trop d'enfants (déterminer
     * selon TAILLE_MAX_ENFANTS_TABLET), nous les stockons dans une table de hachage afin
     * d'accélérer les requêtes.
     */
    struct StockageEnfants {
        kuri::tablet<Noeud *, TAILLE_MAX_ENFANTS_TABLET> enfants{};
        kuri::table_hachage<TypeIdentifiant, Noeud *> table{"Noeud Trie"};

        Noeud *trouve_noeud_pour_identifiant(TypeIdentifiant const &identifiant)
        {
            POUR (enfants) {
                if (it->identifiant == identifiant) {
                    return it;
                }
            }

            return nullptr;
        }

        int64_t taille() const;

        void ajoute(Noeud *noeud)
        {
            enfants.ajoute(noeud);
        }
    };

    struct Noeud {
        /* Identifiant du noeud. */
        TypeIdentifiant identifiant{};
        /* Données stockées dans ce noeud. */
        TypeDonnées données{};
        Noeud *parent = nullptr;
        StockageEnfants enfants{};

        Noeud *trouve_noeud_pour_identifiant(TypeIdentifiant const &ident)
        {
            return enfants.trouve_noeud_pour_identifiant(ident);
        }

        void ajoute_enfant(Noeud *enfant)
        {
            enfants.ajoute(enfant);
            enfant->parent = this;
        }
    };

    Noeud *racine = nullptr;
    tableau_page<Noeud> noeuds{};

    using TypeRésultat = std::variant<Noeud *, TypeDonnées>;

    TypeRésultat trouve_valeur_ou_noeud_insertion(
        kuri::tableau_statique<TypeIdentifiant> identifiants)
    {
        if (!racine) {
            racine = noeuds.ajoute_element();
        }

        auto noeud_courant = racine;
        POUR (identifiants) {
            auto enfant = noeud_courant->trouve_noeud_pour_identifiant(it);
            if (!enfant) {
                enfant = noeuds.ajoute_element();
                enfant->identifiant = it;
                noeud_courant->ajoute_enfant(enfant);
            }
            noeud_courant = enfant;
        }

        if (noeud_courant->données) {
            return noeud_courant->données;
        }

        return noeud_courant;
    }

    int64_t mémoire_utilisée() const
    {
        auto résultat = int64_t(0);
        résultat += noeuds.memoire_utilisee();
        POUR_TABLEAU_PAGE (noeuds) {
            résultat += it.enfants.table.taille_mémoire();
        }
        return résultat;
    }
};

}  // namespace kuri
