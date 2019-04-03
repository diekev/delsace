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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "fenetre_principale.h"

#include <danjo/danjo.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#pragma GCC diagnostic pop

#include "bibliotheques/commandes/repondant_commande.h"

#include "bibloc/logeuse_memoire.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/mikisa.h"
#include "coeur/tache.h"

#include "barre_progres.hh"
#include "editrice_ligne_temps.h"
#include "editrice_noeud.h"
#include "editrice_proprietes.h"
#include "editrice_rendu.h"
#include "editrice_vue2d.h"
#include "editrice_vue3d.h"

static const char *chemins_scripts[] = {
	"entreface/menu_fichier.jo",
	"entreface/menu_edition.jo",
	"entreface/menu_ajouter_noeud_composite.jo",
	"entreface/menu_ajouter_noeud_point3d.jo",
	"entreface/menu_ajouter_noeud_objet.jo",
	"entreface/menu_ajouter_noeud_scene.jo",
	"entreface/menu_ajouter_noeud_simulation.jo",
	"entreface/menu_debogage.jo",
};

enum {
	EDITRICE_PROPRIETE,
	EDITRICE_GRAPHE,
	EDITRICE_LIGNE_TEMPS,
	EDITRICE_RENDU,
	EDITRICE_VUE2D,
	EDITRICE_VUE3D,
};

FenetrePrincipale::FenetrePrincipale(Mikisa &mikisa, QWidget *parent)
	: QMainWindow(parent)
	, m_mikisa(mikisa)
	, m_barre_progres(new BarreDeProgres(m_mikisa, this))
{
	mikisa.fenetre_principale = this;
	mikisa.notifiant_thread = memoire::loge<TaskNotifier>("TaskNotifier", this);

	genere_barre_menu();

	statusBar()->addWidget(m_barre_progres);
	m_barre_progres->setVisible(false);

	auto dock_vue2D = ajoute_dock("Vue 2D", EDITRICE_VUE2D, Qt::LeftDockWidgetArea);
	ajoute_dock("Vue 3D", EDITRICE_VUE3D, Qt::LeftDockWidgetArea, dock_vue2D);
	dock_vue2D->raise();

	ajoute_dock("Grapĥe", EDITRICE_GRAPHE, Qt::LeftDockWidgetArea);
	ajoute_dock("Propriétés", EDITRICE_PROPRIETE, Qt::RightDockWidgetArea);
	ajoute_dock("Rendu", EDITRICE_RENDU, Qt::RightDockWidgetArea);
	ajoute_dock("Ligne Temps", EDITRICE_LIGNE_TEMPS, Qt::RightDockWidgetArea);

	charge_reglages();

	setCentralWidget(nullptr);
}

void FenetrePrincipale::charge_reglages()
{
	QSettings settings;

	auto const &recent_files = settings.value("projet_récents").toStringList();

	for (auto const &file : recent_files) {
		if (QFile(file).exists()) {
			m_mikisa.ajoute_fichier_recent(file.toStdString());
		}
	}
}

void FenetrePrincipale::ecrit_reglages() const
{
	QSettings settings;
	QStringList recent;

	for (auto const &fichier_recent : m_mikisa.fichiers_recents()) {
		recent.push_front(fichier_recent.c_str());
	}

	settings.setValue("projet_récents", recent);
}

void FenetrePrincipale::mis_a_jour_menu_fichier_recent()
{
	std::vector<danjo::DonneesAction> donnees_actions;

	danjo::DonneesAction donnees{};
	donnees.attache = "ouvrir_fichier_recent";
	donnees.repondant_bouton = m_mikisa.repondant_commande();

	for (auto const &fichier_recent : m_mikisa.fichiers_recents()) {
		auto name = QFileInfo(fichier_recent.c_str()).fileName();

		donnees.nom = name.toStdString();
		donnees.metadonnee = fichier_recent;

		donnees_actions.push_back(donnees);
	}

	m_mikisa.gestionnaire_entreface->recree_menu("Projets Récents", donnees_actions);
}

void FenetrePrincipale::closeEvent(QCloseEvent *)
{
	ecrit_reglages();
}

void FenetrePrincipale::genere_barre_menu()
{
	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_mikisa.repondant_commande();

	for (auto const &chemin : chemins_scripts) {
		auto const texte_entree = danjo::contenu_fichier(chemin);
		auto menu = m_mikisa.gestionnaire_entreface->compile_menu(donnees, texte_entree.c_str());

		menuBar()->addMenu(menu);
	}

	auto menu_fichiers_recents = m_mikisa.gestionnaire_entreface->pointeur_menu("Projets Récents");
	connect(menu_fichiers_recents, SIGNAL(aboutToShow()),
			this, SLOT(mis_a_jour_menu_fichier_recent()));
}

QDockWidget *FenetrePrincipale::ajoute_dock(QString const &nom, int type, int aire, QDockWidget *premier)
{
	BaseEditrice *editrice = nullptr;

	switch (type) {
		case EDITRICE_GRAPHE:
			editrice = new EditriceGraphe(m_mikisa);
			break;
		case EDITRICE_PROPRIETE:
			editrice = new EditriceProprietes(m_mikisa);
			break;
		case EDITRICE_LIGNE_TEMPS:
			editrice = new EditriceLigneTemps(m_mikisa);
			break;
		case EDITRICE_RENDU:
			editrice = new EditriceRendu(m_mikisa);
			break;
		case EDITRICE_VUE2D:
			editrice = new EditriceVue2D(m_mikisa);
			break;
		case EDITRICE_VUE3D:
			editrice = new EditriceVue3D(m_mikisa);
			break;
	}

	auto dock = new QDockWidget(nom, this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

	editrice->ajourne_etat(type_evenement::rafraichissement);

	dock->setWidget(editrice);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(static_cast<Qt::DockWidgetArea>(aire), dock);

	if (premier) {
		tabifyDockWidget(premier, dock);
	}

	return dock;
}

void FenetrePrincipale::image_traitee()
{
	m_mikisa.notifie_observatrices(type_evenement::image | type_evenement::traite);
}

void FenetrePrincipale::signale_proces(int quoi)
{
	m_mikisa.notifie_observatrices(quoi);
}

void FenetrePrincipale::tache_demarree()
{
	m_barre_progres->ajourne_valeur(0);
	m_barre_progres->setVisible(true);
}

void FenetrePrincipale::ajourne_progres(float progres)
{
	m_barre_progres->ajourne_valeur(static_cast<int>(progres));
}

void FenetrePrincipale::tache_terminee()
{
	m_barre_progres->setVisible(false);
}

void FenetrePrincipale::evaluation_debutee(const char *message)
{
	m_barre_progres->ajourne_valeur(0);
	m_barre_progres->ajourne_message(message);
}
