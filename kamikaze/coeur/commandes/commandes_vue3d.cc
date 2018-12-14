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

#include <QKeyEvent>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/vision/camera.h"

#include "sdk/context.h"
#include "kamikaze_main.h"
#include "scene.h"

/* ************************************************************************** */

class CommandeZoomCamera : public Commande {
public:
	CommandeZoomCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		const auto delta = donnees.x;

		auto camera = contexte.camera;

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

		contexte.scene->notify_listeners(static_cast<type_evenement>(-1));

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeTourneCamera : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandeTourneCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto camera = contexte.camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		camera->tete(camera->tete() + dy * camera->vitesse_chute());
		camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		camera->besoin_ajournement(true);

		contexte.scene->notify_listeners(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

class CommandePanCamera : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandePanCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto camera = contexte.camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
		camera->cible(camera->cible() + cible);
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		contexte.scene->notify_listeners(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

class CommandeSelectionneObjet : public Commande {
public:
	CommandeSelectionneObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		const auto x = donnees.x;
		const auto y = donnees.y;

		auto camera = contexte.camera;
		const auto &MV = camera->MV();
		const auto &P = camera->P();

#if 1
		const auto &fenetre = dls::math::vec4f(
								  0.0f,
								  0.0f,
								  static_cast<float>(camera->largeur()),
								  static_cast<float>(camera->hauteur()));

		const auto &debut = dls::math::deprojette(
								dls::math::vec3f(x, static_cast<float>(camera->hauteur()) - y, 0.0f),
								MV,
								P,
								fenetre);

		const auto &fin   = dls::math::deprojette(
								dls::math::vec3f(x, static_cast<float>(camera->hauteur()) - y, 1.0f),
								MV,
								P,
								fenetre);

		Ray ray;
		ray.pos = camera->pos();
		ray.dir = dls::math::normalise(fin - debut);

		contexte.scene->entresect(ray);
#else
		float z;
		glReadPixels(x, m_hauteur - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

		const auto pos = dls::math::deprojette(dls::math::vec3f(x, camera->hauteur() - y, z), MV, P, dls::math::vec4f(0, 0, camera->largeur(), camera->hauteur()));
		contexte.scene->selectObject(pos);
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSupprimeObjet : public Commande {
public:
	CommandeSupprimeObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;

		if (scene->active_node() == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		scene->removeObject(scene->active_node());

		contexte.scene->notify_listeners(type_evenement::objet | type_evenement::enleve);

		return EXECUTION_COMMANDE_REUSSIE;
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

	usine.enregistre_type("commande_selectionne_objet",
						   description_commande<CommandeSelectionneObjet>(
							   "vue_3d", Qt::LeftButton, 0, 0, false));

	usine.enregistre_type("commande_supprime_objet",
						   description_commande<CommandeSupprimeObjet>(
							   "vue_3d", 0, 0, Qt::Key_Delete, false));
}
