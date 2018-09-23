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
#include <math/conversion_point_vecteur.h>
#include <math/vec2.h>
#include <QKeyEvent>
#include <glm/gtc/matrix_transform.hpp>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/objets/creation.h"
#include "bibliotheques/vision/camera.h"

#include "../evenement.h"
#include "../composite.h"
#include "../manipulatrice.h"
#include "../mikisa.h"

/* ************************************************************************** */

class CommandeZoomCamera3D : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);
		const auto delta = donnees.y;

		auto camera = mikisa->camera_3d;

		if (delta >= 0) {
			const auto distance = camera->distance() + camera->vitesse_zoom();
			camera->distance(distance);
		}
		else {
			const auto temp = camera->distance() - camera->vitesse_zoom();
			const auto distance = glm::max(0.0f, temp);
			camera->distance(distance);
		}

		camera->ajuste_vitesse();
		camera->besoin_ajournement(true);

		mikisa->notifie_auditeurs(type_evenement::camera_3d | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeTourneCamera3D : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandeTourneCamera3D() = default;

	int execute(void */*pointeur*/, const DonneesCommande &donnees) override
	{
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);
		auto camera = mikisa->camera_3d;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		camera->tete(camera->tete() + dy * camera->vitesse_chute());
		camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		mikisa->notifie_auditeurs(type_evenement::camera_3d | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandePanCamera3D : public Commande {
	float m_vieil_x = 0.0f;
	float m_vieil_y = 0.0f;

public:
	CommandePanCamera3D() = default;

	int execute(void */*pointeur*/, const DonneesCommande &donnees) override
	{
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);
		auto camera = mikisa->camera_3d;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
		camera->cible(camera->cible() + cible);
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		mikisa->notifie_auditeurs(type_evenement::camera_3d | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeSurvoleScene : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);
		auto manipulatrice = mikisa->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* génère un rayon sous la souris */
		auto camera = mikisa->camera_3d;

		auto pos = numero7::math::point3f(
					   donnees.x / camera->largeur(),
					   1.0f - (donnees.y / camera->hauteur()),
					   0.0f);

		const auto debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		const auto fin = camera->pos_monde(pos);

		const auto orig = numero7::math::point3f(
							  camera->pos().x,
							  camera->pos().y,
							  camera->pos().z);

		const auto dir = normalise(fin - debut);

		/* entresecte la manipulatrice */
		const auto etat = manipulatrice->etat();

		manipulatrice->entresecte(orig, dir);

		if (etat != manipulatrice->etat()) {
			mikisa->notifie_auditeurs(type_evenement::camera_3d | type_evenement::modifie);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static Plan plans[] = {
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
};

class CommandeDeplaceManipulatrice : public Commande {
	numero7::math::vec3f m_delta = numero7::math::vec3f(0.0f);

public:
	CommandeDeplaceManipulatrice() = default;

	int execute(void *pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);

		if (mikisa->manipulation_3d_activee == false) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto manipulatrice = mikisa->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* génère un rayon sous la souris */
		auto camera = mikisa->camera_3d;

		auto pos = numero7::math::point3f(
					   donnees.x / camera->largeur(),
					   1.0f - (donnees.y / camera->hauteur()),
					   0.0f);

		const auto debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		const auto fin = camera->pos_monde(pos);

		const auto orig = numero7::math::point3f(
							  camera->pos().x,
							  camera->pos().y,
							  camera->pos().z);

		const auto dir = normalise(fin - debut);

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
			auto dir_cam = mikisa->camera_3d->dir();
			plans[PLAN_XYZ].nor = numero7::math::vec3f(dir_cam.x, dir_cam.y, dir_cam.z);
		}

		if (plan == -1) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		float t;
		plans[plan].pos = manipulatrice->pos();

		const auto &ok = entresecte_plan(plans[plan], orig, dir, t);

		if (ok) {
			const auto &pos = orig + t * dir;
			m_delta = pos - manipulatrice->pos();
		}

		/* ajourne la rotation et la taille originales */
		mikisa->manipulatrice_3d->rotation(mikisa->manipulatrice_3d->rotation());
		mikisa->manipulatrice_3d->taille(mikisa->manipulatrice_3d->taille());

		mikisa->notifie_auditeurs(type_evenement::objet | type_evenement::manipule);

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(void *pointeur, const DonneesCommande &donnees)
	{
		auto mikisa = static_cast<Mikisa *>(pointeur);

		if (mikisa->manipulation_3d_activee == false) {
			return;
		}

		auto manipulatrice = mikisa->manipulatrice_3d;

		if (manipulatrice == nullptr) {
			return;
		}

		if (manipulatrice->etat() == ETAT_DEFAUT) {
			return;
		}

		/* génère un rayon sous la souris */
		auto camera = mikisa->camera_3d;

		auto pos = numero7::math::point3f(
					   donnees.x / camera->largeur(),
					   1.0f - (donnees.y / camera->hauteur()),
					   0.0f);

		const auto debut = camera->pos_monde(pos);

		pos[2] = 1.0f;
		const auto fin = camera->pos_monde(pos);

		const auto orig = numero7::math::point3f(
							  camera->pos().x,
							  camera->pos().y,
							  camera->pos().z);

		const auto dir = normalise(fin - debut);

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
		const auto &ok = entresecte_plan(plans[plan], orig, dir, t);

		if (ok) {
			const auto &pos = vecteur_depuis_point(orig + t * dir);
			manipulatrice->repond_manipulation(pos - m_delta);
		}

		/* ajourne l'opératrice */
		auto graphe = mikisa->graphe;
		auto noeud_actif = graphe->noeud_actif;
		auto operatrice = static_cast<OperatriceImage *>(noeud_actif->donnees());
		operatrice->ajourne_selon_manipulatrice_3d(mikisa->type_manipulation_3d, mikisa->temps_courant);

		mikisa->notifie_auditeurs(type_evenement::objet | type_evenement::manipule);
	}
};

/* ************************************************************************** */

void enregistre_commandes_vue3d(UsineCommande *usine)
{
	usine->enregistre_type("commande_zoom_camera_3d",
						   description_commande<CommandeZoomCamera3D>(
							   "vue_3d", Qt::MiddleButton, 0, 0, true));

	usine->enregistre_type("commande_tourne_camera_3d",
						   description_commande<CommandeTourneCamera3D>(
							   "vue_3d", Qt::MiddleButton, 0, 0, false));

	usine->enregistre_type("commande_pan_camera_3d",
						   description_commande<CommandePanCamera3D>(
							   "vue_3d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

	usine->enregistre_type("commande_survole_scene",
						   description_commande<CommandeSurvoleScene>(
							   "vue_3d", 0, 0, 0, false));

	usine->enregistre_type("commande_deplace_manipulatrice_3d",
						   description_commande<CommandeDeplaceManipulatrice>(
							   "vue_3d", Qt::LeftButton, 0, 0, false));
}
