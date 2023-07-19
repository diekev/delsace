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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "commandes_vue2d.h"

#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "commande_kanba.hh"

#include "../evenement.h"
#include "../kanba.h"
#include "../maillage.h"

/* ************************************************************************** */

template <typename T>
T distance_carree(T x1, T y1, T x2, T y2)
{
    auto x = x1 - x2;
    auto y = y1 - y2;

    return x * x + y * y;
}

class CommandePeinture2D : public CommandeKanba {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
#if 0
		std::cerr << __func__ << '\n';
		std::cerr << "Position <" << donnees.x << ',' << donnees.y << ">\n";
#endif
        auto maillage = kanba.donne_maillage();
        if (maillage == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto calque = maillage->calque_actif();

        if (calque == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        if ((calque->drapeaux & KNB::CALQUE_VERROUILLÉ) != 0) {
            // À FAIRE : message d'erreur dans la barre d'état.
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto brosse = kanba.donne_brosse();
        auto tampon = static_cast<dls::math::vec4f *>(calque->tampon);
        auto &canaux = maillage->canaux_texture();
        auto const largeur = canaux.largeur;
        auto const hauteur = canaux.hauteur;

        auto const rayon_brosse = 10;
        auto const rayon_carre = rayon_brosse * rayon_brosse;
        auto const couleur_brosse = dls::math::vec4f(1.0f, 0.0f, 1.0f, 1.0f);

        for (int i = -rayon_brosse; i < rayon_brosse; ++i) {
            for (int j = -rayon_brosse; j < rayon_brosse; ++j) {
                auto const x = int(donnees.x) + i;
                auto const y = int(donnees.y) + j;

                if (x < 0 || x >= largeur) {
                    continue;
                }

                if (y < 0 || y >= hauteur) {
                    continue;
                }

                if (distance_carree(x, y, int(donnees.x), int(donnees.y)) > rayon_carre) {
                    continue;
                }

                tampon[x + y * largeur] = couleur_brosse;
            }
        }

        kanba.notifie_observatrices(KNB::TypeÉvènement::DESSIN | KNB::TypeÉvènement::FINI);

        return EXECUTION_COMMANDE_MODALE;
    }

    void ajourne_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        execute_kanba(kanba, donnees);
    }

    void termine_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees) override
    {
        execute_kanba(kanba, donnees);
    }
};

/* ************************************************************************** */

void enregistre_commandes_vue2d(UsineCommande &usine)
{
    usine.enregistre_type(
        "commande_peinture_2d",
        description_commande<CommandePeinture2D>("vue_2d", Qt::LeftButton, 0, 0, false));
}
