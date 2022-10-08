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

#include "tableau.hh"

namespace kuri {

enum class DecisionIteration { Arrete, Continue };

template <typename T>
class ensemble {
  private:
    kuri::tableau<T, int> cles{};
    kuri::tableau<char, int> occupes{};

    long capacite = 0;
    long nombre_elements = 0;

    static constexpr auto TAILLE_MIN = 32;

  public:
    void insere(T const &cle)
    {
        auto empreinte = std::hash<T>()(cle);
        auto index = trouve_index_innoccupe(cle, empreinte);
        occupes[index] = 1;
        cles[index] = cle;
    }

    void insere(T &&cle)
    {
        auto empreinte = std::hash<T>()(cle);
        auto index = trouve_index_innoccupe(cle, empreinte);
        occupes[index] = 1;
        cles[index] = std::move(cle);
    }

    void supprime(T const &cle)
    {
        auto empreinte = std::hash<T>()(cle);
        auto index = trouve_index(cle, empreinte);
        if (index != -1) {
            occupes[index] = 0;
            nombre_elements -= 1;
        }
    }

    bool possede(T const &cle) const
    {
        auto empreinte = std::hash<T>()(cle);
        return trouve_index(cle, empreinte) != -1;
    }

    long taille() const
    {
        return nombre_elements;
    }

    long taille_memoire() const
    {
        return occupes.taille_memoire() + cles.taille_memoire();
    }

    bool est_vide() const
    {
        return taille() == 0;
    }

    void efface()
    {
        POUR (occupes) {
            it = 0;
        }
        occupes.efface();
        cles.efface();
        capacite = 0;
        nombre_elements = 0;
    }

    template <typename Rappel>
    void pour_chaque_element(Rappel &&rappel)
    {
        if (capacite == 0) {
            return;
        }

        for (int i = 0; i < capacite; ++i) {
            if (!occupes[i]) {
                continue;
            }

            rappel(cles[i]);
        }
    }

    template <typename Rappel>
    void pour_chaque_element(Rappel &&rappel) const
    {
        if (capacite == 0) {
            return;
        }

        for (int i = 0; i < capacite; ++i) {
            if (!occupes[i]) {
                continue;
            }

            if (rappel(cles[i]) == DecisionIteration::Arrete) {
                break;
            }
        }
    }

  private:
    void alloue(long taille)
    {
        capacite = taille;

        cles.redimensionne(static_cast<int>(taille));
        occupes.redimensionne(static_cast<int>(taille));
        nombre_elements = 0;

        POUR (occupes) {
            it = 0;
        }
    }

    void agrandis()
    {
        auto vieilles_cles = cles;
        auto vieilles_occupes = occupes;

        auto nouvelle_taille = capacite * 2;

        if (nouvelle_taille < TAILLE_MIN) {
            nouvelle_taille = TAILLE_MIN;
        }

        alloue(nouvelle_taille);

        for (auto i = 0; i < vieilles_cles.taille(); ++i) {
            if (vieilles_occupes[i]) {
                insere(std::move(vieilles_cles[i]));
            }
        }
    }

    int trouve_index(T const &cle, size_t empreinte) const
    {
        if (capacite == 0) {
            return -1;
        }

        auto index = static_cast<int>(empreinte % static_cast<size_t>(capacite));

        while (occupes[index]) {
            if (cles[index] == cle) {
                return index;
            }

            index += 1;

            if (index >= capacite) {
                index = 0;
            }
        }

        return -1;
    }

    int trouve_index_innoccupe(T const &cle, size_t empreinte)
    {
        auto index = trouve_index(cle, empreinte);

        if (index == -1) {
            if (nombre_elements * 2 >= capacite) {
                agrandis();
            }

            index = static_cast<int>(empreinte % static_cast<size_t>(capacite));

            while (occupes[index]) {
                index += 1;

                if (index >= capacite) {
                    index = 0;
                }
            }

            nombre_elements += 1;
        }

        return index;
    }
};

}  // namespace kuri
