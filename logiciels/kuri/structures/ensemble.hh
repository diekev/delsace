/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "tableau.hh"

namespace kuri {

enum class DécisionItération { Arrête, Continue };

template <typename T>
class ensemble {
  private:
    kuri::tableau<T, int> cles{};
    kuri::tableau<char, int> occupés{};

    int64_t capacité = 0;
    int64_t nombre_elements = 0;

    static constexpr auto TAILLE_MIN = 32;

  public:
    void insère(T const &clé)
    {
        auto empreinte = std::hash<T>()(clé);
        auto indice = trouve_indice_inoccupé(clé, empreinte);
        occupés[indice] = 1;
        cles[indice] = clé;
    }

    void insère(T &&clé)
    {
        auto empreinte = std::hash<T>()(clé);
        auto indice = trouve_indice_inoccupé(clé, empreinte);
        occupés[indice] = 1;
        cles[indice] = std::move(clé);
    }

    void supprime(T const &clé)
    {
        auto empreinte = std::hash<T>()(clé);
        auto indice = trouve_indice(clé, empreinte);
        if (indice != -1) {
            occupés[indice] = 0;
            nombre_elements -= 1;
        }
    }

    bool possède(T const &clé) const
    {
        auto empreinte = std::hash<T>()(clé);
        return trouve_indice(clé, empreinte) != -1;
    }

    int64_t taille() const
    {
        return nombre_elements;
    }

    int64_t taille_mémoire() const
    {
        return occupés.taille_mémoire() + cles.taille_mémoire();
    }

    bool est_vide() const
    {
        return taille() == 0;
    }

    void efface()
    {
        POUR (occupés) {
            it = 0;
        }
        occupés.efface();
        cles.efface();
        capacité = 0;
        nombre_elements = 0;
    }

    template <typename Rappel>
    void pour_chaque_element(Rappel &&rappel)
    {
        if (capacité == 0) {
            return;
        }

        for (int i = 0; i < capacité; ++i) {
            if (!occupés[i]) {
                continue;
            }

            rappel(cles[i]);
        }
    }

    template <typename Rappel>
    void pour_chaque_element(Rappel &&rappel) const
    {
        if (capacité == 0) {
            return;
        }

        for (int i = 0; i < capacité; ++i) {
            if (!occupés[i]) {
                continue;
            }

            if (rappel(cles[i]) == DécisionItération::Arrête) {
                break;
            }
        }
    }

    kuri::tableau<T> donne_tableau() const
    {
        kuri::tableau<T> résultat;
        pour_chaque_element([&](auto &e) -> DécisionItération {
            résultat.ajoute(e);
            return DécisionItération::Continue;
        });
        return résultat;
    }

  private:
    void alloue(int64_t taille)
    {
        capacité = taille;

        cles.redimensionne(static_cast<int>(taille));
        occupés.redimensionne(static_cast<int>(taille));
        nombre_elements = 0;

        POUR (occupés) {
            it = 0;
        }
    }

    void agrandis()
    {
        auto vieilles_cles = cles;
        auto vieilles_occupés = occupés;

        auto nouvelle_taille = capacité * 2;

        if (nouvelle_taille < TAILLE_MIN) {
            nouvelle_taille = TAILLE_MIN;
        }

        alloue(nouvelle_taille);

        for (auto i = 0; i < vieilles_cles.taille(); ++i) {
            if (vieilles_occupés[i]) {
                insère(std::move(vieilles_cles[i]));
            }
        }
    }

    int trouve_indice(T const &clé, size_t empreinte) const
    {
        if (capacité == 0) {
            return -1;
        }

        auto indice = static_cast<int>(empreinte % static_cast<size_t>(capacité));

        while (occupés[indice]) {
            if (cles[indice] == clé) {
                return indice;
            }

            indice += 1;

            if (indice >= capacité) {
                indice = 0;
            }
        }

        return -1;
    }

    int trouve_indice_inoccupé(T const &clé, size_t empreinte)
    {
        auto indice = trouve_indice(clé, empreinte);

        if (indice == -1) {
            if (nombre_elements * 2 >= capacité) {
                agrandis();
            }

            indice = static_cast<int>(empreinte % static_cast<size_t>(capacité));

            while (occupés[indice]) {
                indice += 1;

                if (indice >= capacité) {
                    indice = 0;
                }
            }

            nombre_elements += 1;
        }

        return indice;
    }
};

template <typename T>
ensemble<T> crée_ensemble(const tableau<T> &tableau)
{
    ensemble<T> résultat;

    POUR (tableau) {
        résultat.insère(it);
    }

    return résultat;
}

}  // namespace kuri
