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

#if 0
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "biblinternes/objets/creation.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/vision/camera.h"

#include "coeur/evenement.h"
#include "coeur/composite.h"
#include "coeur/manipulatrice.h"
#include "coeur/jorjala.hh"
#include "coeur/operatrice_image.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeZoomCamera3D : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		auto const delta = donnees.y;

		auto camera = jorjala->camera_3d;

		if (delta >= 0) {
			auto const distance = camera->distance() + camera->vitesse_zoom();
			camera->distance(distance);
		}
		else {
			auto const temp = camera->distance() - camera->vitesse_zoom();
			auto const distance = std::max(0.0f, temp);
			camera->distance(distance);
		}

		camera->ajuste_vitesse();
		camera->besoin_ajournement(true);

		jorjala->notifie_observatrices(type_evenement::camera_3d | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeTourneCamera3D : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandeTourneCamera3D() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		auto camera = jorjala->camera_3d;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		camera->tete(camera->tete() + dy * camera->vitesse_chute());
		camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		jorjala->notifie_observatrices(type_evenement::camera_3d | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandePanCamera3D : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandePanCamera3D() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		auto camera = jorjala->camera_3d;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
		camera->cible(camera->cible() + cible);
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		jorjala->notifie_observatrices(type_evenement::camera_3d | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeSurvoleScene : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		auto manipulatrice = jorjala->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* génère un rayon sous la souris */
		auto camera = jorjala->camera_3d;

		auto pos = dls::math::point3f(
					   donnees.x / static_cast<float>(camera->largeur()),
					   1.0f - (donnees.y / static_cast<float>(camera->hauteur())),
					   0.0f);

		auto const debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		auto const fin = camera->pos_monde(pos);

		auto const orig = dls::math::point3f(camera->pos());

		auto const dir = normalise(fin - debut);

		/* entresecte la manipulatrice */
		auto const etat = manipulatrice->etat();

		manipulatrice->entresecte(orig, dir);

		if (etat != manipulatrice->etat()) {
			jorjala->notifie_observatrices(type_evenement::camera_3d | type_evenement::modifie);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static Plan plans[] = {
	{dls::math::point3f{0.0f, 0.0f, 0.0f}, dls::math::vec3f{0.0f, 0.0f, 1.0f}},
	{dls::math::point3f{0.0f, 0.0f, 0.0f}, dls::math::vec3f{0.0f, 1.0f, 0.0f}},
	{dls::math::point3f{0.0f, 0.0f, 0.0f}, dls::math::vec3f{1.0f, 0.0f, 0.0f}},
	{dls::math::point3f{0.0f, 0.0f, 0.0f}, dls::math::vec3f{1.0f, 1.0f, 1.0f}},
};

class CommandeDeplaceManipulatrice : public Commande {
	dls::math::vec3f m_delta = dls::math::vec3f(0.0f);

public:
	CommandeDeplaceManipulatrice() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);

		if (jorjala->manipulation_3d_activee == false) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto manipulatrice = jorjala->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* génère un rayon sous la souris */
		auto camera = jorjala->camera_3d;

		auto pos = dls::math::point3f(
					   donnees.x / static_cast<float>(camera->largeur()),
					   1.0f - (donnees.y / static_cast<float>(camera->hauteur())),
					   0.0f);

		auto const debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		auto const fin = camera->pos_monde(pos);

		auto const orig = dls::math::point3f(camera->pos());

		auto const dir = normalise(fin - debut);

		/* entresecte la manipulatrice */
		if (!manipulatrice->entresecte(orig, dir)) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		int plan = -1;

		if (manipulatrice->etat() == ETAT_INTERSECTION_X) {
			plan = PLAN_XY;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_Y) {
			plan = PLAN_XY;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_Z) {
			plan = PLAN_YZ;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_XYZ) {
			plan = PLAN_XYZ;
			auto dir_cam = jorjala->camera_3d->dir();
			plans[PLAN_XYZ].nor = dls::math::vec3f(dir_cam.x, dir_cam.y, dir_cam.z);
		}

		if (plan == -1) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		float t;
		plans[plan].pos = manipulatrice->pos();

		auto const &ok = entresecte_plan(plans[plan], orig, dir, t);

		if (ok) {
			auto const &pos1 = orig + t * dir;
			m_delta = pos1 - manipulatrice->pos();
		}

		/* ajourne la rotation et la taille originales */
		jorjala->manipulatrice_3d->rotation(jorjala->manipulatrice_3d->rotation());
		jorjala->manipulatrice_3d->taille(jorjala->manipulatrice_3d->taille());

		jorjala->notifie_observatrices(type_evenement::objet | type_evenement::manipule);

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);

		if (jorjala->manipulation_3d_activee == false) {
			return;
		}

		auto manipulatrice = jorjala->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return;
		}

		if (manipulatrice->etat() == ETAT_DEFAUT) {
			return;
		}

		/* génère un rayon sous la souris */
		auto camera = jorjala->camera_3d;

		auto pos = dls::math::point3f(
					   donnees.x / static_cast<float>(camera->largeur()),
					   1.0f - (donnees.y / static_cast<float>(camera->hauteur())),
					   0.0f);

		auto const debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		auto const fin = camera->pos_monde(pos);

		auto const orig = dls::math::point3f(camera->pos());

		auto const dir = normalise(fin - debut);

		int plan = -1;

		if (manipulatrice->etat() == ETAT_INTERSECTION_X) {
			plan = PLAN_XY;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_Y) {
			plan = PLAN_XY;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_Z) {
			plan = PLAN_YZ;
		}
		else if (manipulatrice->etat() == ETAT_INTERSECTION_XYZ) {
			plan = PLAN_XYZ;
		}

		if (plan == -1) {
			return;
		}

		float t;
		auto const &ok = entresecte_plan(plans[plan], orig, dir, t);

		if (ok) {
			auto const &pos1 = dls::math::vec3f(orig) + t * dir;
			manipulatrice->repond_manipulation(pos1 - m_delta);
		}

		/* ajourne l'opératrice */
		auto graphe = jorjala->graphe;
		auto noeud_actif = graphe->noeud_actif;
		auto operatrice = extrait_opimage(noeud_actif->donnees);
		operatrice->ajourne_selon_manipulatrice_3d(jorjala->type_manipulation_3d, jorjala->temps_courant);

		/* Évalue tout le graphe pour ajourner proprement les données dépendants
		 * de la transformation de l'objet. */
		jorjala->ajourne_pour_nouveau_temps("fin manipulation déplacement");

		jorjala->notifie_observatrices(type_evenement::objet | type_evenement::manipule);
	}
};
#endif

/* ************************************************************************** */

void enregistre_commandes_vue3d(UsineCommande &usine)
{
#if 0
	usine.enregistre_type("commande_zoom_camera_3d",
						   description_commande<CommandeZoomCamera3D>(
							   "vue_3d", Qt::MiddleButton, 0, 0, true));

	usine.enregistre_type("commande_tourne_camera_3d",
						   description_commande<CommandeTourneCamera3D>(
							   "vue_3d", Qt::MiddleButton, 0, 0, false));

	usine.enregistre_type("commande_pan_camera_3d",
						   description_commande<CommandePanCamera3D>(
							   "vue_3d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

	usine.enregistre_type("commande_survole_scene",
						   description_commande<CommandeSurvoleScene>(
							   "vue_3d", 0, 0, 0, false));

	usine.enregistre_type("commande_deplace_manipulatrice_3d",
						   description_commande<CommandeDeplaceManipulatrice>(
							   "vue_3d", Qt::LeftButton, 0, 0, false));
#endif
}

#pragma clang diagnostic pop
