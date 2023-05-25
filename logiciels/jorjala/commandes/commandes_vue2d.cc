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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"

#include "commande_jorjala.hh"

#include "coeur/jorjala.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeZoomCamera2D final : public CommandeJorjala {
  public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto camera = jorjala.caméra_2d();

        auto zoom = camera.zoom() *
                    ((donnees.y < 0) ? constantes<float>::PHI_INV : constantes<float>::PHI);
        camera.zoom(zoom);
        camera.ajourne_matrice();

        jorjala.notifie_observatrices(JJL::TypeEvenement::CAMÉRA_2D | JJL::TypeEvenement::MODIFIÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandePanCamera2D final : public CommandeJorjala {
    float m_vieil_x = 0.0f;
    float m_vieil_y = 0.0f;

  public:
    CommandePanCamera2D() = default;

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;
        return EXECUTION_COMMANDE_MODALE;
    }

    void ajourne_execution_modale_jorjala(JJL::Jorjala &jorjala,
                                          DonneesCommande const &donnees) override
    {
        auto camera = jorjala.caméra_2d();

        auto delta_pos_x = (m_vieil_x - donnees.x) / static_cast<float>(camera.largeur());
        auto delta_pos_y = (m_vieil_y - donnees.y) / static_cast<float>(camera.hauteur());
        camera.pos_x(camera.pos_x() + delta_pos_x);
        camera.pos_y(camera.pos_y() + delta_pos_y);
        camera.ajourne_matrice();

        m_vieil_x = donnees.x;
        m_vieil_y = donnees.y;

        jorjala.notifie_observatrices(JJL::TypeEvenement::CAMÉRA_2D | JJL::TypeEvenement::MODIFIÉ);
    }

    JJL::TypeCurseur type_curseur_modal() override
    {
        return JJL::TypeCurseur::MAIN_FERMÉE;
    }
};

/* ************************************************************************** */

#if 0
struct CommandeOutil2D final : public CommandeJorjala {
	float vieil_x = 0.0f;
	float vieil_y = 0.0f;

	int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
		/* vérifie si le clique est dans l'image */

		auto const &noeud_composite = jorjala->bdd.graphe_composites()->noeud_actif;

		if (noeud_composite == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto composite = extrait_composite(noeud_composite->donnees);
		auto const &image = composite->image();

		auto calque = image.calque_pour_lecture("image");

		if (calque == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        auto camera_2d = jorjala.caméra_2d();
		/* converti en espace -1:1 */
		auto x = (static_cast<float>(camera_2d->largeur) - donnees.x);
		auto y = (static_cast<float>(camera_2d->hauteur) - donnees.y);

		x -= static_cast<float>(camera_2d->largeur / 2);
		y -= static_cast<float>(camera_2d->hauteur / 2);

		/* converti en espace monde */
		x = (x / camera_2d->zoom) - camera_2d->pos_x;
		y = (y / camera_2d->zoom) - camera_2d->pos_y;

		std::cerr << "Pos écran : " << donnees.x << ", " << donnees.y << '\n';
		std::cerr << "Pos monde : " << x << ", " << y << '\n';

		auto co = dls::math::vec2f(x, y);

		auto tampon = extrait_grille_couleur(calque);

		if (tampon->hors_des_limites(tampon->monde_vers_index(co))) {
			std::cerr << "Le clique est HORS des limites du calque...\n";
		}
		else {
			std::cerr << "Le clique est DANS les limites du calque...\n";
		}

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);
		INUTILISE(donnees);
	}
};
#endif

/* ************************************************************************** */

void enregistre_commandes_vue2d(UsineCommande &usine)
{
    usine.enregistre_type(
        "commande_zoom_camera_2d",
        description_commande<CommandeZoomCamera2D>("vue_2d", Qt::MiddleButton, 0, 0, true));

    usine.enregistre_type("commande_pan_camera_2d",
                          description_commande<CommandePanCamera2D>(
                              "vue_2d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

#if 0
	usine.enregistre_type("commande_outil_2d",
						   description_commande<CommandeOutil2D>(
							   "vue_2d", Qt::LeftButton, 0, 0, false));
#endif
}

#pragma clang diagnostic pop
