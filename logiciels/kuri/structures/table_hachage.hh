/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

#include "utilitaires/macros.hh"

namespace kuri {

#undef COMPTE_COLLISION

template <typename Clé, typename Valeur, int FACTEUR_DE_CHARGE = 70>
struct table_hachage {
  private:
    kuri::tableau<Clé, int> cles{};
    kuri::tableau<Valeur, int> valeurs{};
    kuri::tableau<char, int> occupés{};
    kuri::tableau<size_t, int> empreintes{};

    int64_t capacité = 0;
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
        os << "-- capacité            " << capacité << '\n';
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
     * Cette approche pose problème, car l'utilisation de l'indice suivant pourrait
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
        return 1 + static_cast<int>(empreinte % (static_cast<size_t>(capacité) - 1));
    }

  public:
    void alloue(int64_t taille)
    {
        capacité = taille;

        cles.redimensionne(static_cast<int>(taille));
        valeurs.redimensionne(static_cast<int>(taille));
        occupés.redimensionne(static_cast<int>(taille));
        empreintes.redimensionne(static_cast<int>(taille));
        nombre_elements = 0;

        POUR (occupés) {
            it = 0;
        }
    }

    void agrandis()
    {
        auto vieilles_cles = cles;
        auto vieilles_valeurs = valeurs;
        auto vieilles_occupés = occupés;

        auto nouvelle_taille = capacité * 2;

        if (nouvelle_taille < TAILLE_MIN) {
            nouvelle_taille = TAILLE_MIN;
        }

        alloue(nouvelle_taille);

        for (auto i = 0; i < vieilles_cles.taille(); ++i) {
            if (vieilles_occupés[i]) {
                insère(std::move(vieilles_cles[i]), std::move(vieilles_valeurs[i]));
            }
        }
    }

    void insère(Clé const &clé, Valeur const &valeur)
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice_inoccupé(clé, empreinte);
        occupés[indice] = 1;
        empreintes[indice] = empreinte;
        cles[indice] = clé;
        valeurs[indice] = valeur;
    }

    void insère(Clé &&clé, Valeur &&valeur)
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice_inoccupé(clé, empreinte);
        occupés[indice] = 1;
        empreintes[indice] = empreinte;
        cles[indice] = std::move(clé);
        valeurs[indice] = std::move(valeur);
    }

    void efface(Clé const &clé)
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);
        if (indice == -1) {
            return;
        }

        occupés[indice] = false;
        empreintes[indice] = 0;
        cles[indice] = Clé();
        valeurs[indice] = Valeur();
        nombre_elements -= 1;
    }

    Valeur trouve(Clé const &clé, bool &trouve) const
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);

        if (indice == -1) {
            trouve = false;
            return {};
        }

        trouve = true;
        return valeurs[indice];
    }

    Valeur &trouve_ref(Clé const &clé)
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);
        return valeurs[indice];
    }

    Valeur *trouve_pointeur(Clé const &clé)
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);

        if (indice == -1) {
            return nullptr;
        }

        return &valeurs[indice];
    }

    Valeur const &trouve_ref(Clé const &clé) const
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);
        return valeurs[indice];
    }

    Valeur valeur_ou(Clé const &clé, Valeur défaut) const
    {
        auto trouvee = false;
        auto valeur = trouve(clé, trouvee);

        if (!trouvee) {
            return défaut;
        }

        return valeur;
    }

    bool possède(Clé const &clé) const
    {
        auto empreinte = std::hash<Clé>()(clé);
        auto indice = trouve_indice(clé, empreinte);
        return indice != -1;
    }

    int trouve_indice(Clé const &clé, size_t empreinte) const
    {
        if (capacité == 0) {
            return -1;
        }

        auto indice = static_cast<int>(empreinte % static_cast<size_t>(capacité));
        auto increment = increment_de_base_pour_empreinte(empreinte);

        while (occupés[indice]) {
            if (empreintes[indice] == empreinte) {
                if (cles[indice] == clé) {
                    return indice;
                }
            }
#ifdef COMPTE_COLLISION
            nombre_de_collision_recherche += 1;
#endif

            indice += increment;
            increment += 1;

            while (indice >= capacité) {
                indice -= static_cast<int>(capacité);
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
        return int64_t(occupés.taille_mémoire()) + int64_t(empreintes.taille_mémoire()) +
               int64_t(cles.taille_mémoire()) + int64_t(valeurs.taille_mémoire());
    }

    void efface()
    {
        occupés.efface();
        empreintes.efface();
        cles.efface();
        valeurs.efface();
        capacité = 0;
        nombre_elements = 0;
    }

    void reinitialise()
    {
        nombre_elements = 0;
        POUR (occupés) {
            it = false;
        }
    }

    template <typename TypeFonction>
    void pour_chaque_élément(TypeFonction &&fonction) const
    {
        POUR_INDICE (occupés) {
            if (!it) {
                continue;
            }

            auto const &valeur = valeurs[indice_it];
            fonction(valeur);
        }
    }

  private:
    int trouve_indice_inoccupé(Clé const &clé, size_t empreinte)
    {
        /* éléments / alloués >= FACTEUR_DE_CHARGE / 100
         * donc
         * éléments * 100 >= alloués * FACTEUR_DE_CHARGE
         * + 1 pour être cohérent sur la division de nombre entiers
         */
        if ((nombre_elements + 1) * 100 >= capacité * FACTEUR_DE_CHARGE) {
            agrandis();
        }

        auto indice = static_cast<int>(empreinte % static_cast<size_t>(capacité));
        auto increment = increment_de_base_pour_empreinte(empreinte);

        while (occupés[indice]) {
#ifdef COMPTE_COLLISION
            nombre_de_collision_ajout += 1;
#endif
            indice += increment;
            increment += 1;

            while (indice >= capacité) {
                indice -= static_cast<int>(capacité);
            }
        }

        nombre_elements += 1;
        return indice;
    }
};

}  // namespace kuri
