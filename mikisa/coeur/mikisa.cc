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

#include "bibloc/logeuse_memoire.hh"

#include "composite.h"
#include "configuration.h"
#include "evaluation.h"
#include "manipulatrice.h"
#include "noeud_image.h"
#include "tache.h"

#include "commandes/commandes_edition.h"
#include "commandes/commandes_noeuds.h"
#include "commandes/commandes_projet.h"
#include "commandes/commandes_rendu.h"
#include "commandes/commandes_temps.h"
#include "commandes/commandes_vue2d.h"
#include "commandes/commandes_vue3d.h"

#include "operatrices/operatrices_3d.h"
#include "operatrices/operatrices_attributs.hh"
#include "operatrices/operatrices_bullet.hh"
#include "operatrices/operatrices_cheveux.h"
#include "operatrices/operatrices_corps.hh"
#include "operatrices/operatrices_flux.h"
#include "operatrices/operatrices_fracture.hh"
#include "operatrices/operatrices_groupes.hh"
#include "operatrices/operatrices_maillage.hh"
#include "operatrices/operatrices_opensubdiv.hh"
#include "operatrices/operatrices_particules.h"
#include "operatrices/operatrices_pixel.h"
#include "operatrices/operatrices_point3d.h"
#include "operatrices/operatrices_region.h"
#include "operatrices/operatrices_simulations.hh"
#include "operatrices/operatrices_vetements.hh"
#include "operatrices/operatrices_visualisation.hh"
#include "operatrices/operatrices_volume.hh"

static constexpr auto MAX_FICHIER_RECENT = 10;

Mikisa::Mikisa()
	: m_usine_commande{}
	, m_usine_operatrices{}
	, m_repondant_commande(memoire::loge<RepondantCommande>("RepondantCommande", m_usine_commande, this))
	, composite(memoire::loge<Composite>("Composite"))
	, fenetre_principale(nullptr)
	, editrice_active(nullptr)
	, gestionnaire_entreface(memoire::loge<danjo::GestionnaireInterface>("danjo::GestionnaireInterface"))
	, project_settings(memoire::loge<ProjectSettings>("ProjectSettings"))
	, camera_2d(memoire::loge<vision::Camera2D>("vision::Camera2D"))
	, camera_3d(memoire::loge<vision::Camera3D>("vision::Camera3D", 0, 0))
	, graphe(&composite->graph())
	, type_manipulation_3d(MANIPULATION_POSITION)
	, chemin_courant("/composite/")
	, notifiant_thread(nullptr)
	, chef_execution(*this)
{
	camera_3d->projection(vision::TypeProjection::PERSPECTIVE);
}

Mikisa::~Mikisa()
{
	memoire::deloge("TaskNotifier", notifiant_thread);
	memoire::deloge("vision::Camera2D", camera_2d);
	memoire::deloge("vision::Camera3D", camera_3d);
	memoire::deloge("Composite", composite);
	memoire::deloge("ProjectSettings", project_settings);
	memoire::deloge("RepondantCommande", m_repondant_commande);
	memoire::deloge("danjo::GestionnaireInterface", gestionnaire_entreface);
}

void Mikisa::initialise()
{
	enregistre_operatrices_3d(m_usine_operatrices);
	enregistre_operatrices_attributs(m_usine_operatrices);
	enregistre_operatrices_bullet(m_usine_operatrices);
	enregistre_operatrices_cheveux(m_usine_operatrices);
	enregistre_operatrices_corps(m_usine_operatrices);
	enregistre_operatrices_flux(m_usine_operatrices);
	enregistre_operatrices_fracture(m_usine_operatrices);
	enregistre_operatrices_groupes(m_usine_operatrices);
	enregistre_operatrices_maillage(m_usine_operatrices);
	enregistre_operatrices_opensubdiv(m_usine_operatrices);
	enregistre_operatrices_particules(m_usine_operatrices);
	enregistre_operatrices_pixel(m_usine_operatrices);
	enregistre_operatrices_point3d(m_usine_operatrices);
	enregistre_operatrices_region(m_usine_operatrices);
	enregistre_operatrices_simulations(m_usine_operatrices);
	enregistre_operatrices_vetement(m_usine_operatrices);
	enregistre_operatrices_visualisation(m_usine_operatrices);
	enregistre_operatrices_volume(m_usine_operatrices);

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

void Mikisa::affiche_erreur(std::string const &message)
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

void Mikisa::chemin_projet(std::string const &chemin)
{
	m_chemin_projet = chemin;
	ajoute_fichier_recent(chemin);
}

void Mikisa::ajoute_fichier_recent(std::string const &chemin)
{
	auto index = std::find(m_fichiers_recents.debut(), m_fichiers_recents.fin(), chemin);

	if (index != m_fichiers_recents.fin()) {
		std::rotate(m_fichiers_recents.debut(), index, index + 1);
	}
	else {
		m_fichiers_recents.insere(m_fichiers_recents.debut(), chemin);

		if (m_fichiers_recents.taille() > MAX_FICHIER_RECENT) {
			m_fichiers_recents.redimensionne(MAX_FICHIER_RECENT);
		}
	}
}

dls::tableau<std::string> const &Mikisa::fichiers_recents()
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

void Mikisa::ajourne_pour_nouveau_temps(const char *message)
{
	evalue_resultat(*this, message);
}
