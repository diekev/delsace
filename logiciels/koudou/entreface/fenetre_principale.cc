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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "fenetre_principale.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QAction>
#include <QDockWidget>
#include <QHelpEvent>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolTip>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/configuration.h"

#include "base_dialogue.h"
#include "editeur_arbre_scene.h"
#include "editeur_camera.h"
#include "editeur_objet.h"
#include "editeur_material.h"
#include "editeur_monde.h"
#include "editeur_canevas.h"
#include "editeur_parametres.h"
#include "editeur_rendu.h"

FenetrePrincipale::FenetrePrincipale(QWidget *parent)
    : QMainWindow(parent)
	, m_dialogue_preferences(new BaseDialogue(m_koudou, this))
	, m_dialogue_parametres_projet(new ProjectSettingsDialog(m_koudou, this))
	, m_progress_bar(new QProgressBar(this))
{
	m_koudou.fenetre_principale = this;

	genere_menu_preference();

	auto menu = menuBar()->addMenu("Fichier");
	auto action = menu->addAction("Ouvrir fichier");
	action->setData("ouvrir_fichier");

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

	statusBar()->addWidget(m_progress_bar);
	m_progress_bar->setRange(0, 100);
	m_progress_bar->setVisible(false);

	ajoute_visionneur_image();
	ajoute_editeur_proprietes();

	setCentralWidget(nullptr);
}

FenetrePrincipale::~FenetrePrincipale()
{
	delete m_viewer_dock;
}

void FenetrePrincipale::genere_menu_preference()
{
	auto menu_edition = menuBar()->addMenu("Édition");

	QAction *action;

	action = menu_edition->addAction("Paramètres projet");
	connect(action, &QAction::triggered, this, &FenetrePrincipale::montre_parametres_projet);

	action = menu_edition->addAction("Préférences");
	connect(action, &QAction::triggered, this, &FenetrePrincipale::montre_preferences);
}

void FenetrePrincipale::ajoute_editeur_proprietes()
{
	/* Rendu */

	auto dock_rendu = new QDockWidget("Rendu", this);
	dock_rendu->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_rendu = new EditriceRendu(m_koudou, dock_rendu);
	editeur_rendu->ajourne_etat(static_cast<type_evenement>(-1));

	dock_rendu->setWidget(editeur_rendu);
	dock_rendu->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_rendu);

	/* Caméra */
#ifdef NOUVELLE_CAMERA
	auto dock_camera = new QDockWidget("Caméra", this);
	dock_camera->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_camera = new EditeurCamera(&m_koudou, dock_camera);
	editeur_camera->ajourne_etat(type_evenement::rafraichissement);

	dock_camera->setWidget(editeur_camera);
	dock_camera->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_camera);
#endif

	/* Monde */

	auto dock_monde = new QDockWidget("Monde", this);
	dock_monde->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_monde = new EditeurMonde(&m_koudou, dock_monde);
	editeur_monde->ajourne_etat(type_evenement::rafraichissement);

	dock_monde->setWidget(editeur_monde);
	dock_monde->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_monde);

	/* Paramètres */

	auto dock_parametres = new QDockWidget("Paramètres", this);
	dock_parametres->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_parametres = new EditeurParametres(&m_koudou, dock_parametres);
	editeur_parametres->ajourne_etat(static_cast<type_evenement>(-1));

	dock_parametres->setWidget(editeur_parametres);
	dock_parametres->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_parametres);

	/* Maillage. */

	auto dock_objet = new QDockWidget("Objet", this);
	dock_objet->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_objet = new EditeurObjet(&m_koudou, dock_objet);
	editeur_objet->ajourne_etat(static_cast<type_evenement>(-1));

	dock_objet->setWidget(editeur_objet);
	dock_objet->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_parametres);

	/* Matérial. */

	auto dock_material = new QDockWidget("Matérial", this);
	dock_material->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_material = new EditeurMaterial(&m_koudou, dock_material);
	editeur_material->ajourne_etat(static_cast<type_evenement>(-1));

	dock_material->setWidget(editeur_material);
	dock_material->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_parametres);

	/* Arbre scène. */

	auto dock_arbre_scene = new QDockWidget("Arbre", this);
	dock_arbre_scene->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_arbre_scene = new EditeurArbreScene(&m_koudou, dock_arbre_scene);
	editeur_arbre_scene->ajourne_etat(static_cast<type_evenement>(-1));

	dock_arbre_scene->setWidget(editeur_arbre_scene);
	dock_arbre_scene->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_parametres);

	/* Agenencement */

	tabifyDockWidget(dock_rendu, dock_parametres);
#ifdef NOUVELLE_CAMERA
	tabifyDockWidget(dock_parametres, dock_camera);
	tabifyDockWidget(dock_camera, dock_monde);
#else
	tabifyDockWidget(dock_parametres, dock_monde);
#endif
	tabifyDockWidget(dock_monde, dock_objet);
	tabifyDockWidget(dock_objet, dock_material);
	tabifyDockWidget(dock_material, dock_arbre_scene);

	/* Fais en sorte que le dock des propriétés soient visible par défaut. */
	dock_rendu->raise();
}

void FenetrePrincipale::ajoute_visionneur_image()
{
	/* TODO: figure out a way to have multiple GL context. */
	if (m_viewer_dock == nullptr) {
		m_viewer_dock = new QDockWidget("Rendu", this);

		auto view_2d = new EditeurCanevas(m_koudou, m_viewer_dock);

		m_viewer_dock->setWidget(view_2d);
		m_viewer_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

		addDockWidget(Qt::LeftDockWidgetArea, m_viewer_dock);

		auto dock_vue_3d = new QDockWidget("Vue 3D", this);
		dock_vue_3d->setAttribute(Qt::WA_DeleteOnClose);

		auto editeur_vue3d = new EditriceVue3D(m_koudou, dock_vue_3d);
		editeur_vue3d->ajourne_etat(static_cast<type_evenement>(-1));

		dock_vue_3d->setWidget(editeur_vue3d);
		dock_vue_3d->setAllowedAreas(Qt::AllDockWidgetAreas);

		addDockWidget(Qt::LeftDockWidgetArea, dock_vue_3d);

		tabifyDockWidget(dock_vue_3d, m_viewer_dock);
		dock_vue_3d->raise();
	}

	m_viewer_dock->show();
}

void FenetrePrincipale::rendu_fini()
{
	m_koudou.notifie_observatrices(type_evenement::rendu | type_evenement::fini);
}

void FenetrePrincipale::tache_commence()
{
	m_progress_bar->setValue(0);
	m_progress_bar->setVisible(true);
}

void FenetrePrincipale::progres_avance(float progress)
{
	m_progress_bar->setValue(static_cast<int>(progress));
}

void FenetrePrincipale::progres_temps(unsigned int echantillon, double temps_echantillon, double temps_ecoule, double temps_restant)
{
	m_koudou.informations_rendu.echantillon = echantillon;
	m_koudou.informations_rendu.temps_echantillon = temps_echantillon;
	m_koudou.informations_rendu.temps_ecoule = temps_ecoule;
	m_koudou.informations_rendu.temps_restant = temps_restant;

	m_koudou.notifie_observatrices(type_evenement::rafraichissement);
}

void FenetrePrincipale::tache_fini()
{
	m_progress_bar->setVisible(false);
}

void FenetrePrincipale::montre_preferences()
{
	m_dialogue_preferences->montre();

	if (m_dialogue_preferences->exec() == QDialog::Accepted) {
		m_dialogue_preferences->ajourne();
	}
}

void FenetrePrincipale::montre_parametres_projet()
{
	m_dialogue_parametres_projet->montre();

	if (m_dialogue_parametres_projet->exec() == QDialog::Accepted) {
		m_dialogue_parametres_projet->ajourne();
	}
}

void FenetrePrincipale::repond_action()
{
	auto action = qobject_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	m_koudou.repondant_commande->repond_clique(action->data().toString().toStdString(), "");
}
