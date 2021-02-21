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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau.hh"

namespace kuri {

template <typename Cle, typename Valeur>
struct table_hachage {
private:
    dls::tableau<Cle> cles{};
    dls::tableau<Valeur> valeurs{};
    dls::tableau<char> occupes{};
    dls::tableau<size_t> empreintes{};

    long capacite = 0;
    long nombre_elements = 0;

    static constexpr auto TAILLE_MIN = 32;

public:
    void alloue(long taille)
    {
        capacite = taille;

        cles.redimensionne(taille);
        valeurs.redimensionne(taille);
        occupes.redimensionne(taille);
        empreintes.redimensionne(taille);

        POUR (occupes) {
            it = 0;
        }
    }

    void agrandis()
    {
        auto vieilles_cles = cles;
        auto vieilles_valeurs = valeurs;
        auto vieilles_empreintes = empreintes;
        auto vieilles_occupes = occupes;

        auto nouvelle_taille = capacite * 2;

        if (nouvelle_taille < TAILLE_MIN) {
            nouvelle_taille = TAILLE_MIN;
        }

        alloue(nouvelle_taille);

        for (auto i = 0; i < vieilles_cles.taille(); ++i) {
            if (vieilles_occupes[i]) {
				insere(std::move(vieilles_cles[i]), std::move(vieilles_valeurs[i]));
            }
        }
    }

    void insere(Cle const &cle, Valeur const &valeur)
    {
		auto empreinte = std::hash<Cle>()(cle);
		auto index = trouve_index_innoccupe(cle, empreinte);
        occupes[index] = 1;
        empreintes[index] = empreinte;
        cles[index] = cle;
        valeurs[index] = valeur;
    }

	void insere(Cle &&cle, Valeur &&valeur)
	{
		auto empreinte = std::hash<Cle>()(cle);
		auto index = trouve_index_innoccupe(cle, empreinte);
		occupes[index] = 1;
		empreintes[index] = empreinte;
		cles[index] = std::move(cle);
		valeurs[index] = std::move(valeur);
	}

    Valeur trouve(Cle const &cle, bool &trouve)
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

	Valeur valeur_ou(Cle const &cle, Valeur defaut)
	{
		auto trouvee = false;
		auto valeur = trouve(cle, trouvee);

		if (!trouvee) {
			return defaut;
		}

		return valeur;
	}

    bool possed(Cle const &cle)
    {
        auto empreinte = calcule_empreinte(cle);
        auto index = trouve_index(cle, empreinte);
        return index != -1;
    }

    long trouve_index(Cle const &cle, size_t empreinte)
    {
        if (capacite == 0) {
            return -1;
        }

        auto index = static_cast<long>(empreinte % static_cast<size_t>(capacite));

        while (occupes[index]) {
            if (empreintes[index] == empreinte) {
                if (cles[index] == cle) {
                    return index;
                }
            }

            index += 1;

            if (index >= capacite) {
                index = 0;
            }
        }

        return -1;
    }

    long taille() const
    {
        return nombre_elements;
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

private:
	long trouve_index_innoccupe(Cle const &cle, size_t empreinte)
	{
		auto index = trouve_index(cle, empreinte);

		if (index == -1) {
			if (nombre_elements * 2 >= capacite) {
				agrandis();
			}

			index = static_cast<long>(empreinte % static_cast<size_t>(capacite));

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

}
