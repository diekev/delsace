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

#include "jorjala.hh"

#if 1

#include "danjo/danjo.h"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "commandes/commandes_edition.h"
#include "commandes/commandes_noeuds.h"
#include "commandes/commandes_objet.hh"
#include "commandes/commandes_projet.h"
#include "commandes/commandes_rendu.h"
#include "commandes/commandes_temps.h"
#include "commandes/commandes_vue2d.h"
#include "commandes/commandes_vue3d.h"

#include "ipa/table_types.c"

static void enregistre_commandes(UsineCommande &usine_commande)
{
    enregistre_commandes_graphes(usine_commande);
    enregistre_commandes_objet(usine_commande);
    enregistre_commandes_projet(usine_commande);
    enregistre_commandes_edition(usine_commande);
    enregistre_commandes_rendu(usine_commande);
    enregistre_commandes_temps(usine_commande);
    enregistre_commandes_vue2d(usine_commande);
    enregistre_commandes_vue3d(usine_commande);
}

static void initialise_données_programme(DonnéesProgramme *données_programme, JJL::Jorjala &jorjala)
{
    jorjala.crée_gestionnaire_fenêtre(données_programme);

    données_programme->gestionnaire_danjo = memoire::loge<danjo::GestionnaireInterface>("danjo::GestionnaireInterface");
    données_programme->usine_commande = memoire::loge<UsineCommande>("UsineCommande");
    données_programme->repondant_commande = memoire::loge<RepondantCommande>("RepondantCommande", *données_programme->usine_commande, jorjala);

    enregistre_commandes(*données_programme->usine_commande);
}

std::optional<JJL::Jorjala> initialise_jorjala()
{
    if (!initialise_jorjala("/opt/bin/jorjala/ipa/jorjala.so")) {
        return {};
    }

    JJL::Jorjala jorjala = JJL::crée_instance_jorjala();
    if (jorjala == nullptr) {
        return {};
    }

    /* Initialise les données pour communiquer avec l'interface. */
    auto données_programme = memoire::loge<DonnéesProgramme>("DonnéesProgramme");
    initialise_données_programme(données_programme, jorjala);

    return jorjala;
}

void issitialise_jorjala(JJL::Jorjala &jorjala)
{
    auto données_programme = accède_données_programme(jorjala);

    memoire::deloge("UsineCommande", données_programme->usine_commande);
    memoire::deloge("RepondantCommande", données_programme->repondant_commande);
    memoire::deloge("danjo::GestionnaireInterface", données_programme->gestionnaire_danjo);

    jorjala.libère_mémoire();
}

DonnéesProgramme *accède_données_programme(JJL::Jorjala &jorjala)
{
    auto données_programme = jorjala.gestionnaire_fenêtre();
    return static_cast<DonnéesProgramme *>(données_programme.données());
}

RepondantCommande *repondant_commande(JJL::Jorjala &jorjala)
{
    auto données = accède_données_programme(jorjala);
    return données->repondant_commande;
}

danjo::GestionnaireInterface *gestionnaire_danjo(JJL::Jorjala &jorjala)
{
    auto données = accède_données_programme(jorjala);
    return données->gestionnaire_danjo;
}

danjo::DonneesInterface cree_donnees_interface_danjo(JJL::Jorjala &jorjala, danjo::Manipulable *manipulable, danjo::ConteneurControles *conteneur)
{
    danjo::DonneesInterface résultat{};
    résultat.conteneur = nullptr;
    résultat.repondant_bouton = repondant_commande(jorjala);
    résultat.manipulable = manipulable;
    return résultat;
}

#else

#include <algorithm>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#include <QMessageBox>
#pragma GCC diagnostic pop

#ifdef AVEC_OPENEXR
#	include <OpenEXR/ImfThreading.h>
#endif

#include "danjo/danjo.h"

#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/vision/camera_2d.h"
#include "biblinternes/vision/camera.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "evaluation/evaluation.hh"

#include "composite.h"
#include "configuration.h"
#include "manipulatrice.h"
#include "noeud_image.h"
#include "rendu.hh"
#include "tache.h"
#include "operatrice_graphe_detail.hh"

#include "lcc/lcc.hh"

#include "operatrices/operatrices_3d.h"
#include "operatrices/operatrices_alembic.hh"
#include "operatrices/operatrices_arbre.hh"
#include "operatrices/operatrices_attributs.hh"
#include "operatrices/operatrices_bullet.hh"
#include "operatrices/operatrices_cheveux.h"
#include "operatrices/operatrices_corps.hh"
#include "operatrices/operatrices_flux.h"
#include "operatrices/operatrices_fracture.hh"
#include "operatrices/operatrices_groupes.hh"
#include "operatrices/operatrices_image_profonde.hh"
#include "operatrices/operatrices_images_3d.hh"
#include "operatrices/operatrices_maillage.hh"
#include "operatrices/operatrices_ocean.hh"
#include "operatrices/operatrices_opensubdiv.hh"
#include "operatrices/operatrices_particules.h"
#include "operatrices/operatrices_pixel.h"
#include "operatrices/operatrices_poseidon.hh"
#include "operatrices/operatrices_region.h"
#include "operatrices/operatrices_rendu.hh"
#include "operatrices/operatrices_script.hh"
#include "operatrices/operatrices_simulations.hh"
#include "operatrices/operatrices_srirp.hh"
#include "operatrices/operatrices_terrain.hh"
#include "operatrices/operatrices_uvs.hh"
#include "operatrices/operatrices_vetements.hh"
#include "operatrices/operatrices_visualisation.hh"
#include "operatrices/operatrices_volume.hh"

static constexpr auto MAX_FICHIER_RECENT = 10;

Jorjala::Jorjala()
	: m_usine_commande{}
	, m_usine_operatrices{}
	, m_repondant_commande(memoire::loge<RepondantCommande>("RepondantCommande", m_usine_commande, this))
	, fenetre_principale(nullptr)
	, editrice_active(nullptr)
	, gestionnaire_entreface(memoire::loge<danjo::GestionnaireInterface>("danjo::GestionnaireInterface"))
	, project_settings(memoire::loge<ProjectSettings>("ProjectSettings"))
	, camera_2d(memoire::loge<vision::Camera2D>("vision::Camera2D"))
	, camera_3d(memoire::loge<vision::Camera3D>("vision::Camera3D", 0, 0))
	, graphe(nullptr)
	, type_manipulation_3d(MANIPULATION_POSITION)
	, chemin_courant("/objets/")
	, notifiant_thread(nullptr)
	, chef_execution(*this)
	, lcc(memoire::loge<lcc::LCC>("LCC"))
{
	graphe = bdd.graphe_objets();

	camera_3d->projection(vision::TypeProjection::PERSPECTIVE);

#ifdef AVEC_OPENEXR
	auto fils = static_cast<int>(std::thread::hardware_concurrency());
	OPENEXR_IMF_NAMESPACE::setGlobalThreadCount(fils);
#endif
}

Jorjala::~Jorjala()
{
	memoire::deloge("LCC", lcc);
	memoire::deloge("TaskNotifier", notifiant_thread);
	memoire::deloge("vision::Camera2D", camera_2d);
	memoire::deloge("vision::Camera3D", camera_3d);
	memoire::deloge("ProjectSettings", project_settings);
	memoire::deloge("RepondantCommande", m_repondant_commande);
	memoire::deloge("danjo::GestionnaireInterface", gestionnaire_entreface);

#ifdef AVEC_OPENEXR
	/* Détruit la mémoire allouée par openexr pour gérer les fils. */
	OPENEXR_IMF_NAMESPACE::setGlobalThreadCount(0);
#endif
}

void Jorjala::initialise()
{
	enregistre_operatrices_3d(m_usine_operatrices);
	enregistre_operatrices_alembic(m_usine_operatrices);
	enregistre_operatrices_arbre(m_usine_operatrices);
	enregistre_operatrices_attributs(m_usine_operatrices);
	enregistre_operatrices_bullet(m_usine_operatrices);
	enregistre_operatrices_cheveux(m_usine_operatrices);
	enregistre_operatrices_corps(m_usine_operatrices);
	enregistre_operatrices_detail(m_usine_operatrices);
	enregistre_operatrices_flux(m_usine_operatrices);
	enregistre_operatrices_fracture(m_usine_operatrices);
	enregistre_operatrices_image_profonde(m_usine_operatrices);
	enregistre_operatrices_images_3d(m_usine_operatrices);
	enregistre_operatrices_groupes(m_usine_operatrices);
	enregistre_operatrices_maillage(m_usine_operatrices);
	enregistre_operatrices_ocean(m_usine_operatrices);
	enregistre_operatrices_opensubdiv(m_usine_operatrices);
	enregistre_operatrices_particules(m_usine_operatrices);
	enregistre_operatrices_pixel(m_usine_operatrices);
	enregistre_operatrices_poseidon(m_usine_operatrices);
	enregistre_operatrices_region(m_usine_operatrices);
	enregistre_operatrices_rendu(m_usine_operatrices);
	enregistre_operatrices_script(m_usine_operatrices);
	enregistre_operatrices_simulations(m_usine_operatrices);
	enregistre_operatrices_srirp(m_usine_operatrices);
	enregistre_operatrices_terrain(m_usine_operatrices);
	enregistre_operatrices_uvs(m_usine_operatrices);
	enregistre_operatrices_vetement(m_usine_operatrices);
	enregistre_operatrices_visualisation(m_usine_operatrices);
    enregistre_operatrices_volume(m_usine_operatrices);

	cree_rendu_defaut(*this);

	lcc::initialise(*lcc);
}

UsineOperatrice &Jorjala::usine_operatrices()
{
	return m_usine_operatrices;
}

void Jorjala::ajourne_pour_nouveau_temps(const char *message)
{
    requiers_evaluation(*this, TEMPS_CHANGE, message);
}

Jorjala::EtatLogiciel Jorjala::etat_courant()
{
	auto etat = EtatLogiciel();

	return etat;
}

void Jorjala::empile_etat()
{
//	if (!pile_refait.est_vide()) {
//		pile_refait.efface();
//	}

//	pile_defait.empile(etat_courant());
}

void Jorjala::defait()
{
//	if (pile_defait.est_vide()) {
//		return;
//	}

//	pile_refait.empile(etat_courant());

//	auto etat = pile_defait.depile();
}

void Jorjala::refait()
{
//	if (pile_refait.est_vide()) {
//		return;
//	}

//	pile_defait.empile(etat_courant());

//	auto etat = pile_refait.depile();
}

#endif
