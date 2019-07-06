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

#include "fenetre_principale.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDockWidget>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/repondant_commande.h"

#include "coeur/evenement.h"

#include "editeur_brosse.h"
#include "editeur_calques.h"
#include "editeur_canevas.h"
#include "editeur_parametres.h"

FenetrePrincipale::FenetrePrincipale(QWidget *parent)
	: QMainWindow(parent)
{
	m_kanba.fenetre_principale = this;

	m_kanba.enregistre_commandes();

	m_progress_bar = new QProgressBar(this);
	statusBar()->addWidget(m_progress_bar);
	m_progress_bar->setRange(0, 100);
	m_progress_bar->setVisible(false);

	ajoute_visionneur_image();
	ajoute_editeur_proprietes();

	setCentralWidget(nullptr);

	auto menu = menuBar()->addMenu("Fichier");
	auto action = menu->addAction("Ouvrir projet");
	action->setData("ouvrir_projet");

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

	action = menu->addAction("Sauvegarder projet");
	action->setData("sauvegarder_projet");

	menu->addSeparator();

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

	action = menu->addAction("Ouvrir fichier");
	action->setData("ouvrir_fichier");

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

	menu = menuBar()->addMenu("Édition");
	action = menu->addAction("Ajouter cube");
	action->setData("ajouter_cube");

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

	action = menu->addAction("Ajouter sphere");
	action->setData("ajouter_sphere");

	connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));
}

FenetrePrincipale::~FenetrePrincipale()
{
	delete m_viewer_dock;
}

void FenetrePrincipale::ajoute_editeur_proprietes()
{
	/* Paramètres */

	auto dock_parametres = new QDockWidget("Paramètres", this);
	dock_parametres->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_parametres = new EditeurParametres(&m_kanba, dock_parametres);
	editeur_parametres->ajourne_etat(static_cast<type_evenement>(-1));

	dock_parametres->setWidget(editeur_parametres);
	dock_parametres->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_parametres);

	/* Calques */

	auto dock_calques = new QDockWidget("Calques", this);
	dock_calques->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_calques = new EditeurCalques(&m_kanba, dock_calques);
	editeur_calques->ajourne_etat(static_cast<type_evenement>(-1));

	dock_calques->setWidget(editeur_calques);
	dock_calques->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_calques);

	/* Brosse */

	auto dock_brosse = new QDockWidget("Brosse", this);
	dock_brosse->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_brosse = new EditeurBrosse(&m_kanba, dock_brosse);
	editeur_brosse->ajourne_etat(static_cast<type_evenement>(-1));

	dock_brosse->setWidget(editeur_brosse);
	dock_brosse->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_brosse);

	tabifyDockWidget(dock_parametres, dock_calques);
	tabifyDockWidget(dock_calques, dock_brosse);
}

void FenetrePrincipale::ajoute_visionneur_image()
{
	/* TODO: figure out a way to have multiple GL context. */
	if (m_viewer_dock == nullptr) {
		m_viewer_dock = new QDockWidget("Visionneur", this);

		auto view_2d = new EditeurCanevas(m_kanba, m_viewer_dock);

		m_viewer_dock->setWidget(view_2d);
		m_viewer_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

		addDockWidget(Qt::TopDockWidgetArea, m_viewer_dock);
	}

	m_viewer_dock->show();
}

void FenetrePrincipale::rendu_fini()
{
	//	auto moteur_rendu = m_kanba.moteur_rendu;
	//	moteur_rendu->notifie_observatrices(type_evenement::rendu | type_evenement::fini);
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

void FenetrePrincipale::progres_temps(int echantillon, float temps_echantillon, float temps_ecoule, float temps_restant)
{
	//	auto moteur_rendu = m_kanba.moteur_rendu;
	//	moteur_rendu->notifie_observatrices(type_evenement::rafraichissement);
}

void FenetrePrincipale::tache_fini()
{
	m_progress_bar->setVisible(false);
}

void FenetrePrincipale::repond_action()
{
	auto action = qobject_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	m_kanba.repondant_commande->repond_clique( action->data().toString().toStdString(), "");
}
