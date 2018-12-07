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

#include "bibliotheques/commandes/repondant_commande.h"

#include "coeur/evenement.h"

#include "editeur_arbre.h"
#include "editeur_canevas.h"

FenetrePrincipale::FenetrePrincipale(QWidget *parent)
	: QMainWindow(parent)
{
	m_silvatheque.fenetre_principale = this;

	m_silvatheque.enregistre_commandes();

	m_progress_bar = new QProgressBar(this);
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

void FenetrePrincipale::ajoute_editeur_proprietes()
{
	/* Brosse */

	auto dock_arbre = new QDockWidget("Arbre", this);
	dock_arbre->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_arbre = new EditeurArbre(&m_silvatheque, dock_arbre);
	editeur_arbre->ajourne_etat(type_evenement::rafraichissement);

	dock_arbre->setWidget(editeur_arbre);
	dock_arbre->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_arbre);
}

void FenetrePrincipale::ajoute_visionneur_image()
{
	/* TODO: figure out a way to have multiple GL context. */
	if (m_viewer_dock == nullptr) {
		m_viewer_dock = new QDockWidget("Visionneur", this);

		auto view_2d = new EditeurCanevas(m_silvatheque, m_viewer_dock);

		m_viewer_dock->setWidget(view_2d);
		m_viewer_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

		addDockWidget(Qt::LeftDockWidgetArea, m_viewer_dock);
	}

	m_viewer_dock->show();
}

void FenetrePrincipale::rendu_fini()
{
	//	auto moteur_rendu = m_silvatheque.moteur_rendu;
	//	moteur_rendu->notifie_auditeurs(type_evenement::rendu | type_evenement::fini);
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
	//	auto moteur_rendu = m_silvatheque.moteur_rendu;
	//	moteur_rendu->notifie_auditeurs(type_evenement::rafraichissement);
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

	m_silvatheque.repondant_commande->repond_clique( action->data().toString().toStdString(), "");
}
