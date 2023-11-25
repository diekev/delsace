/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

#include "utilitaires/macros.hh"

namespace kuri {

#undef COMPTE_COLLISION

template <typename Cle, typename Valeur, int FACTEUR_DE_CHARGE = 70>
struct table_hachage {
  private:
    kuri::tableau<Cle, int> cles{};
    kuri::tableau<Valeur, int> valeurs{};
    kuri::tableau<char, int> occupes{};
    kuri::tableau<size_t, int> empreintes{};

    int64_t capacite = 0;
    int64_t nombre_elements = 0;

    static constexpr auto TAILLE_MIN = 32;

    const char *nom = nullptr;

#ifdef COMPTE_COLLISION
    int nombre_de_collision_recherche = 0;
    int nombre_de_collision_ajout = 0;
#endif

  public:
    COPIE_CONSTRUCT(table_hachage);

    explicit table_hachage(const char *identifiant) : nom(identifiant)
    {
    }

    ~table_hachage()
    {
#ifdef COMPTE_COLLISION
        auto &os = std::cout;

        os << "Table \"" << nom << "\"\n";
        os << "-- capacité            " << capacite << '\n';
        os << "-- éléments            " << nombre_elements << '\n';
        os << "-- collision recherche " << nombre_de_collision_recherche << '\n';
        os << "-- collision ajout     " << nombre_de_collision_ajout << '\n';
        os << "-- collision / élément "
           << (static_cast<double>(nombre_de_collision_ajout) / nombre_elements) << '\n';
#endif
    }

  private:
    /* Utilise un incrément différent pour chaque empreinte.
     *
     * Quand nous insérons ou recherchons une valeur, nous commençons par
     * la position correspond à l'empreinte. Si elle est occupée par
     * quelque chose n'ayant pas la même empreinte (ou simplement par
     * quelque chose lors d'une insertion), nous allons à la position suivante.
     *
     * Cette approche pose problème, car l'utilisation de l'index suivant pourrait
     * causer des collisions futures avec des objets n'ayant pas la même empreinte
     * mais devant se trouver à cette position.
     *
     * Pour éviter d'avoir des régions contiguës trop grande, et éviter trop
     * de collisions, nous utilisons un décalage différent pour chaque
     * empreinte : si il y a une collision, la position suivante sera
     * `position de base + incrément`.
     *
     * L'incrément est également incrément à chaque collision.
     *
     * Visuellement, sans incrément nous aurions (où les valeurs A B C D E
     * auraient la même empreinte, ou une empreinte différente mais la position
     * fut déjà utilisée) :
     *
     * _ _ _ _ A B C D E _ _ _ _ _ _ _ _ _ _
     *
     * Avec :
     *
     * _ _ _ _ A _ B _ _ C _ _ _ D _ _ _ _ E
     */
    inline int increment_de_base_pour_empreinte(size_t empreinte) const
    {
        /* - 1 pour être relativement premier avec la capacité. */
        return 1 + static_cast<int>(empreinte % (static_cast<size_t>(capacite) - 1));
    }

  public:
    void alloue(int64_t taille)
    {
        capacite = taille;

        cles.redimensionne(static_cast<int>(taille));
        valeurs.redimensionne(static_cast<int>(taille));
        occupes.redimensionne(static_cast<int>(taille));
        empreintes.redimensionne(static_cast<int>(taille));
        nombre_elements = 0;

        POUR (occupes) {
            it = 0;
        }
    }

    void agrandis()
    {
        auto vieilles_cles = cles;
        auto vieilles_valeurs = valeurs;
        auto vieilles_occupes = occupes;

        auto nouvelle_taille = capacite * 2;

        if (nouvelle_taille < TAILLE_MIN) {
            nouvelle_taille = TAILLE_MIN;
        }

        alloue(nouvelle_taille);

        for (auto i = 0; i < vieilles_cles.taille(); ++i) {
            if (vieilles_occupes[i]) {
                insère(std::move(vieilles_cles[i]), std::move(vieilles_valeurs[i]));
            }
        }
    }

    void insère(Cle const &cle, Valeur const &valeur)
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index_innoccupe(cle, empreinte);
        occupes[index] = 1;
        empreintes[index] = empreinte;
        cles[index] = cle;
        valeurs[index] = valeur;
    }

    void insère(Cle &&cle, Valeur &&valeur)
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index_innoccupe(cle, empreinte);
        occupes[index] = 1;
        empreintes[index] = empreinte;
        cles[index] = std::move(cle);
        valeurs[index] = std::move(valeur);
    }

    void efface(Cle const &cle)
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index(cle, empreinte);
        if (index == -1) {
            return;
        }

        occupes[index] = false;
        empreintes[index] = 0;
        cles[index] = Cle();
        valeurs[index] = Valeur();
        nombre_elements -= 1;
    }

    Valeur trouve(Cle const &cle, bool &trouve) const
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index(cle, empreinte);

        if (index == -1) {
            trouve = false;
            return {};
        }

        trouve = true;
        return valeurs[index];
    }

    Valeur &trouve_ref(Cle const &cle)
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index(cle, empreinte);
        return valeurs[index];
    }

    Valeur *trouve_pointeur(Cle const &clé)
    {
        auto empreinte = std::hash<Cle>()(clé);
        auto index = trouve_index(clé, empreinte);

        if (index == -1) {
            return nullptr;
        }

        return &valeurs[index];
    }

    Valeur const &trouve_ref(Cle const &cle) const
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index(cle, empreinte);
        return valeurs[index];
    }

    Valeur valeur_ou(Cle const &cle, Valeur defaut) const
    {
        auto trouvee = false;
        auto valeur = trouve(cle, trouvee);

        if (!trouvee) {
            return defaut;
        }

        return valeur;
    }

    bool possède(Cle const &cle) const
    {
        auto empreinte = std::hash<Cle>()(cle);
        auto index = trouve_index(cle, empreinte);
        return index != -1;
    }

    int trouve_index(Cle const &cle, size_t empreinte) const
    {
        if (capacite == 0) {
            return -1;
        }

        auto index = static_cast<int>(empreinte % static_cast<size_t>(capacite));
        auto increment = increment_de_base_pour_empreinte(empreinte);

        while (occupes[index]) {
            if (empreintes[index] == empreinte) {
                if (cles[index] == cle) {
                    return index;
                }
            }
#ifdef COMPTE_COLLISION
            nombre_de_collision_recherche += 1;
#endif

            index += increment;
            increment += 1;

            while (index >= capacite) {
                index -= static_cast<int>(capacite);
            }
        }

        return -1;
    }

    int64_t taille() const
    {
        return nombre_elements;
    }

    int64_t taille_mémoire() const
    {
        return int64_t(occupes.taille_memoire()) + int64_t(empreintes.taille_memoire()) +
               int64_t(cles.taille_memoire()) + int64_t(valeurs.taille_memoire());
    }

    void efface()
    {
        occupes.efface();
        empreintes.efface();
        cles.efface();
        valeurs.efface();
        capacite = 0;
        nombre_elements = 0;
    }

    void reinitialise()
    {
        nombre_elements = 0;
        POUR (occupes) {
            it = false;
        }
    }

    template <typename TypeFonction>
    void pour_chaque_élément(TypeFonction &&fonction)
    {
        POUR_INDEX (occupes) {
            if (!it) {
                continue;
            }

            auto const &valeur = valeurs[index_it];
            fonction(valeur);
        }
    }

  private:
    int trouve_index_innoccupe(Cle const &cle, size_t empreinte)
    {
        /* éléments / alloués >= FACTEUR_DE_CHARGE / 100
         * donc
         * éléments * 100 >= alloués * FACTEUR_DE_CHARGE
         * + 1 pour être cohérent sur la division de nombre entiers
         */
        if ((nombre_elements + 1) * 100 >= capacite * FACTEUR_DE_CHARGE) {
            agrandis();
        }

        auto index = static_cast<int>(empreinte % static_cast<size_t>(capacite));
        auto increment = increment_de_base_pour_empreinte(empreinte);

        while (occupes[index]) {
#ifdef COMPTE_COLLISION
            nombre_de_collision_ajout += 1;
#endif
            index += increment;
            increment += 1;

            while (index >= capacite) {
                index -= static_cast<int>(capacite);
            }
        }

        nombre_elements += 1;
        return index;
    }
};

}  // namespace kuri
