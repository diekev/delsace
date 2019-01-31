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

#include "mikisa.h"

#include <algorithm>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#include <QMessageBox>
#pragma GCC diagnostic pop

#include <danjo/danjo.h>

#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/vision/camera_2d.h"
#include "bibliotheques/vision/camera.h"

#include "composite.h"
#include "configuration.h"
#include "evaluation.h"
#include "manipulatrice.h"
#include "noeud_image.h"

#include "commandes/commandes_edition.h"
#include "commandes/commandes_noeuds.h"
#include "commandes/commandes_projet.h"
#include "commandes/commandes_rendu.h"
#include "commandes/commandes_temps.h"
#include "commandes/commandes_vue2d.h"
#include "commandes/commandes_vue3d.h"

#include "operatrices/operatrices_3d.h"
#include "operatrices/operatrices_cheveux.h"
#include "operatrices/operatrices_flux.h"
#include "operatrices/operatrices_particules.h"
#include "operatrices/operatrices_pixel.h"
#include "operatrices/operatrices_point3d.h"
#include "operatrices/operatrices_region.h"
#include "operatrices/operatrices_simulations.hh"

static constexpr auto MAX_FICHIER_RECENT = 10;

Mikisa::Mikisa()
	: m_usine_commande{}
	, m_usine_operatrices{}
	, m_repondant_commande(new RepondantCommande(m_usine_commande, this))
	, composite(new Composite)
	, fenetre_principale(nullptr)
	, editrice_active(nullptr)
	, gestionnaire_entreface(new danjo::GestionnaireInterface)
	, project_settings(new ProjectSettings)
	, camera_2d(new vision::Camera2D())
	, camera_3d(new vision::Camera3D(0, 0))
	, graphe(&composite->graph())
	, type_manipulation_3d(MANIPULATION_POSITION)
	, chemin_courant("/composite/")
{
	camera_3d->projection(vision::TypeProjection::PERSPECTIVE);
}

Mikisa::~Mikisa()
{
	delete camera_2d;
	delete camera_3d;
	delete composite;
	delete project_settings;
	delete m_repondant_commande;
	delete gestionnaire_entreface;
}

void Mikisa::initialise()
{
	enregistre_operatrices_3d(m_usine_operatrices);
	enregistre_operatrices_cheveux(m_usine_operatrices);
	enregistre_operatrices_flux(m_usine_operatrices);
	enregistre_operatrices_particules(m_usine_operatrices);
	enregistre_operatrices_pixel(m_usine_operatrices);
	enregistre_operatrices_point3d(m_usine_operatrices);
	enregistre_operatrices_region(m_usine_operatrices);
	enregistre_operatrices_simulations(m_usine_operatrices);

	enregistre_commandes_graphes(m_usine_commande);
	enregistre_commandes_projet(m_usine_commande);
	enregistre_commandes_edition(m_usine_commande);
	enregistre_commandes_rendu(m_usine_commande);
	enregistre_commandes_temps(m_usine_commande);
	enregistre_commandes_vue2d(m_usine_commande);
	enregistre_commandes_vue3d(m_usine_commande);
}

UsineCommande &Mikisa::usine_commandes()
{
	return m_usine_commande;
}

UsineOperatrice &Mikisa::usine_operatrices()
{
	return m_usine_operatrices;
}

std::string Mikisa::requiers_dialogue(int type)
{
	/* À FAIRE : sort ça de la classe. */
	if (type == FICHIER_OUVERTURE) {
		auto const chemin = QFileDialog::getOpenFileName();
		return chemin.toStdString();
	}

	if (type == FICHIER_SAUVEGARDE) {
		auto const chemin = QFileDialog::getSaveFileName();
		return chemin.toStdString();
	}

	return "";
}

void Mikisa::affiche_erreur(const std::string &message)
{
	/* À FAIRE : sort ça de la classe. */
	QMessageBox boite_message;
	boite_message.critical(nullptr, "Erreur", message.c_str());
	boite_message.setFixedSize(500, 200);
}

std::string Mikisa::chemin_projet() const
{
	return m_chemin_projet;
}

void Mikisa::chemin_projet(const std::string &chemin)
{
	m_chemin_projet = chemin;
	ajoute_fichier_recent(chemin);
}

void Mikisa::ajoute_fichier_recent(const std::string &chemin)
{
	auto index = std::find(m_fichiers_recents.begin(), m_fichiers_recents.end(), chemin);

	if (index != m_fichiers_recents.end()) {
		std::rotate(m_fichiers_recents.begin(), index, index + 1);
	}
	else {
		m_fichiers_recents.insert(m_fichiers_recents.begin(), chemin);

		if (m_fichiers_recents.size() > MAX_FICHIER_RECENT) {
			m_fichiers_recents.resize(MAX_FICHIER_RECENT);
		}
	}
}

const std::vector<std::string> &Mikisa::fichiers_recents()
{
	return m_fichiers_recents;
}

bool Mikisa::projet_ouvert() const
{
	return m_projet_ouvert;
}

void Mikisa::projet_ouvert(bool ouinon)
{
	m_projet_ouvert = ouinon;
}

RepondantCommande *Mikisa::repondant_commande() const
{
	return m_repondant_commande;
}

void Mikisa::ajourne_pour_nouveau_temps()
{
	evalue_resultat(*this);
}
