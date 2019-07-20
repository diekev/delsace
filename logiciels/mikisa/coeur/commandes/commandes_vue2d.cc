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

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/vision/camera_2d.h"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/mikisa.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeZoomCamera2D final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto camera = mikisa->camera_2d;

		camera->zoom *= (donnees.y < 0) ? constantes<float>::PHI_INV : constantes<float>::PHI;
		camera->ajourne_matrice();

		mikisa->notifie_observatrices(type_evenement::camera_2d | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandePanCamera2D final : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandePanCamera2D() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto camera = mikisa->camera_2d;

		camera->pos_x += (m_vieil_x - donnees.x) / static_cast<float>(camera->largeur);
		camera->pos_y += (m_vieil_y - donnees.y) / static_cast<float>(camera->hauteur);
		camera->ajourne_matrice();

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		mikisa->notifie_observatrices(type_evenement::camera_2d | type_evenement::modifie);
	}
};

/* ************************************************************************** */

void enregistre_commandes_vue2d(UsineCommande &usine)
{
	usine.enregistre_type("commande_zoom_camera_2d",
						   description_commande<CommandeZoomCamera2D>(
							   "vue_2d", Qt::MiddleButton, 0, 0, true));

	usine.enregistre_type("commande_pan_camera_2d",
						   description_commande<CommandePanCamera2D>(
							   "vue_2d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));
}

#pragma clang diagnostic pop
