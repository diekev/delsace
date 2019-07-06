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

#include "commandes_vue3d.h"

#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/commande.h"
#include "biblinternes/objets/creation.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/vision/camera.h"

#include "../evenement.h"
#include "../poseidon.h"

/* ************************************************************************** */

class CommandeZoomCamera : public Commande {
public:
	CommandeZoomCamera() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto poseidon = std::any_cast<Poseidon *>(pointeur);
		auto const delta = donnees.x;

		auto camera = poseidon->camera;

		if (delta >= 0) {
			auto distance = camera->distance() + camera->vitesse_zoom();
			camera->distance(distance);
		}
		else {
			const float temp = camera->distance() - camera->vitesse_zoom();
			auto distance = std::max(0.0f, temp);
			camera->distance(distance);
		}

		camera->ajuste_vitesse();
		camera->besoin_ajournement(true);

		poseidon->notifie_observatrices(static_cast<type_evenement>(-1));

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeTourneCamera : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandeTourneCamera() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto poseidon = std::any_cast<Poseidon *>(pointeur);
		auto camera = poseidon->camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		camera->tete(camera->tete() + dy * camera->vitesse_chute());
		camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		poseidon->notifie_observatrices(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

class CommandePanCamera : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandePanCamera() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto poseidon = std::any_cast<Poseidon *>(pointeur);
		auto camera = poseidon->camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
		camera->cible(camera->cible() + cible);
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		poseidon->notifie_observatrices(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

void enregistre_commandes_vue3d(UsineCommande &usine)
{
	usine.enregistre_type("commande_zoom_camera",
						   description_commande<CommandeZoomCamera>(
							   "vue_3d", Qt::MiddleButton, 0, 0, true));

	usine.enregistre_type("commande_tourne_camera",
						   description_commande<CommandeTourneCamera>(
							   "vue_3d", Qt::MiddleButton, 0, 0, false));

	usine.enregistre_type("commande_pan_camera",
						   description_commande<CommandePanCamera>(
							   "vue_3d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));
}
