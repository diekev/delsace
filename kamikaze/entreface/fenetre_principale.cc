﻿/*
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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "fenetre_principale.h"

#include "sdk/primitive.h"
#include <danjo/danjo.h>
#include <danjo/repondant_bouton.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDockWidget>
#include <QFileInfo>
#include <QMenuBar>
#include <QProgressBar>
#include <QStatusBar>
#include <QSettings>
#pragma GCC diagnostic pop

#include <iostream>

#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/vision/camera.h"

#include "coeur/kamikaze_main.h"

#include "editeur_arborescence.h"
#include "editeur_canevas.h"
#include "editeur_graphe.h"
#include "editeur_ligne_temps.h"
#include "editeur_proprietes.h"

static const char *chemins_scripts[] = {
	"entreface/menu_fichier.jo",
//	"entreface/menu_edition.jo",
	"entreface/menu_objet.jo",
	"entreface/menu_debogage.jo",
};

FenetrePrincipale::FenetrePrincipale(Main *main, QWidget *parent)
    : QMainWindow(parent)
	, m_main(main)
	, m_contexte(m_main->usine_operatrice())
	, m_repondant_commande(new RepondantCommande(main->usine_commandes(), main))
	, m_gestionnaire(new danjo::GestionnaireInterface)
	, m_barre_progres(new QProgressBar(this))
{
	genere_menu_fichier();
	genere_menu_noeud();
	genere_menu_fenetre();
	genere_menu_prereglages();

	statusBar()->addWidget(m_barre_progres);
	m_barre_progres->setRange(0, 100);
	m_barre_progres->setVisible(false);

	/* setup context */
	m_contexte_evaluation.edit_mode = false;
	m_contexte_evaluation.animation = false;
	m_main->contexte.eval_ctx = &m_contexte_evaluation;
	m_main->contexte.scene = m_main->scene();
	m_main->contexte.primitive_factory = m_main->primitive_factory();
	m_main->contexte.usine_operatrice = m_main->usine_operatrice();
	m_main->contexte.main_window = this;
	m_main->contexte.active_widget = nullptr;
	m_main->contexte.camera = new vision::Camera3D(0, 0);

	ajout_editeur_canevas();
	ajout_editeur_propriete();
	ajout_editeurs_arborescence_graphe();
	ajout_editeur_ligne_temps();

	setCentralWidget(nullptr);

	charge_reglages();
}

FenetrePrincipale::~FenetrePrincipale()
{
	delete m_gestionnaire;
	delete m_repondant_commande;
}

void FenetrePrincipale::taskStarted()
{
	m_barre_progres->setValue(0);
	m_barre_progres->setVisible(true);
}

void FenetrePrincipale::updateProgress(float progress)
{
	m_barre_progres->setValue(static_cast<int>(progress));
}

void FenetrePrincipale::taskEnded()
{
	m_barre_progres->setVisible(false);
}

void FenetrePrincipale::nodeProcessed()
{
	m_main->contexte.scene->notify_listeners(type_evenement::noeud | type_evenement::traite);
}

void FenetrePrincipale::genere_menu_fenetre()
{
	auto menu = menuBar()->addMenu("Fenêtre");

	QAction *action;

	action = menu->addAction("Arborescence");
	action->setToolTip("Ajouter une Arborescence");
	connect(action, SIGNAL(triggered()), this, SLOT(ajout_editeur_arborescence()));

	action = menu->addAction("Canevas 3D");
	action->setToolTip("Ajouter un Canevas 3D");
	connect(action, SIGNAL(triggered()), this, SLOT(ajout_editeur_canevas()));

	action = menu->addAction("Éditeur de Graphe");
	action->setToolTip("Ajouter un Éditeur de Graphe");
	connect(action, SIGNAL(triggered()), this, SLOT(ajout_editeur_graphe()));

	action = menu->addAction("Éditeur de Ligne de Temps");
	action->setToolTip("Ajouter un Éditeur de Ligne de Temps");
	connect(action, SIGNAL(triggered()), this, SLOT(ajout_editeur_ligne_temps()));

	action = menu->addAction("Éditeur de Propriétés");
	action->setToolTip("Ajouter un Éditeur de Propriétés");
	connect(action, SIGNAL(triggered()), this, SLOT(ajout_editeur_propriete()));
}

void FenetrePrincipale::genere_menu_fichier()
{
	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_repondant_commande;

	for (auto const &chemin : chemins_scripts) {
		auto const texte_entree = danjo::contenu_fichier(chemin);
		auto menu = m_gestionnaire->compile_menu(donnees, texte_entree.c_str());

		if (menu == nullptr) {
			std::cerr << "Impossible de générer le menu pour " << chemin << '\n';
		}

		menuBar()->addMenu(menu);
	}

	auto menu_fichiers_recents = m_gestionnaire->pointeur_menu("Projets Récents");
	connect(menu_fichiers_recents, SIGNAL(aboutToShow()),
			this, SLOT(mis_a_jour_menu_fichier_recent()));
}

void FenetrePrincipale::genere_menu_noeud()
{
	auto categories = m_main->usine_operatrice().categories();

	std::stringstream ss;

	ss << "menu \"Ajoute Noeud\" {\n";

	for (auto const &categorie : categories) {
		ss << "\tmenu \"" << categorie << "\" {\n";

		auto cles = m_main->usine_operatrice().cles(categorie);

		std::sort(cles.begin(), cles.end(),
				  [](DescOperatrice const &desc1, DescOperatrice const &desc2)
		{
			return desc1.nom < desc2.nom;
		});

		for (auto const &description : cles) {
			ss << "\t\taction(valeur=\"" << description.nom
			   << "\"; attache=ajouter_noeud; métadonnée=\""
			   << description.nom << "\")\n";
		}

		ss << "\t}\n";
	}

	ss << "}";

	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_repondant_commande;

	m_menu_ajout_noeud = m_gestionnaire->compile_menu(donnees, ss.str().c_str());
	menuBar()->addMenu(m_menu_ajout_noeud);
}

void FenetrePrincipale::genere_menu_prereglages()
{
	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_repondant_commande;

	auto const texte_entree = danjo::contenu_fichier("entreface/menu_prereglage.jo");

	auto barre_outil = m_gestionnaire->compile_barre_outils(donnees, texte_entree.c_str());
	addToolBar(Qt::TopToolBarArea, barre_outil);
}

void FenetrePrincipale::charge_reglages()
{
	QSettings settings;

	auto const &recent_files = settings.value("projet_récents").toStringList();

	for (auto const &file : recent_files) {
		if (QFile(file).exists()) {
			m_main->ajoute_fichier_recent(file.toStdString());
		}
	}
}

void FenetrePrincipale::ecrit_reglages() const
{
	QSettings settings;
	QStringList recent;

	for (auto const &fichier_recent : m_main->fichiers_recents()) {
		recent.push_front(fichier_recent.c_str());
	}

	settings.setValue("projet_récents", recent);
}

void FenetrePrincipale::closeEvent(QCloseEvent *)
{
	ecrit_reglages();
}

void FenetrePrincipale::ajout_editeur_arborescence()
{
	auto dock = new QDockWidget("Arborescence", this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur = new EditeurArborescence(dock);
	editeur->listens(&m_main->contexte);
	editeur->update_state(type_evenement::objet | type_evenement::ajoute);

	dock->setWidget(editeur);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void FenetrePrincipale::ajout_editeur_canevas()
{
	/* TODO: figure out a way to have multiple GL context. */
	if (m_dock_canevas == nullptr) {
		m_dock_canevas = new QDockWidget("Canevas", this);

		auto editeur = new EditeurCanvas(m_repondant_commande, m_dock_canevas);
		editeur->listens(&m_main->contexte);
		editeur->update_state(static_cast<type_evenement>(-1));

		m_dock_canevas->setWidget(editeur);
		m_dock_canevas->setAllowedAreas(Qt::AllDockWidgetAreas);

		addDockWidget(Qt::LeftDockWidgetArea, m_dock_canevas);
	}

	m_dock_canevas->show();
}

void FenetrePrincipale::ajout_editeur_graphe()
{
	auto dock = new QDockWidget("Éditeur Graphe", this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur = new EditriceGraphe(m_repondant_commande, m_gestionnaire, dock);
	editeur->listens(&m_main->contexte);
	editeur->menu_ajout_noeud(m_menu_ajout_noeud);
	editeur->update_state(static_cast<type_evenement>(-1));

	dock->setWidget(editeur);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock);
}

void FenetrePrincipale::ajout_editeur_ligne_temps()
{
	auto dock = new QDockWidget("Ligne de Temps", this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur = new EditriceLigneTemps(m_repondant_commande, dock);
	editeur->listens(&m_main->contexte);
	editeur->update_state(type_evenement::temps | type_evenement::modifie);

	dock->setWidget(editeur);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock);
}

void FenetrePrincipale::ajout_editeur_propriete()
{
	auto dock = new QDockWidget("Propriétés", this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur = new EditriceProprietes(dock);
	editeur->listens(&m_main->contexte);
	editeur->update_state(static_cast<type_evenement>(-1));

	dock->setWidget(editeur);
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock);
}

void FenetrePrincipale::ajout_editeurs_arborescence_graphe()
{
	auto dock_arborescence = new QDockWidget("Arborescence", this);
	dock_arborescence->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_arborescence = new EditeurArborescence(dock_arborescence);
	editeur_arborescence->listens(&m_main->contexte);
	editeur_arborescence->update_state(type_evenement::objet | type_evenement::ajoute);

	dock_arborescence->setWidget(editeur_arborescence);
	dock_arborescence->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_arborescence);

	auto dock_graphe = new QDockWidget("Éditeur Graphe", this);
	dock_graphe->setAttribute(Qt::WA_DeleteOnClose);

	auto editeur_graphe = new EditriceGraphe(m_repondant_commande, m_gestionnaire, dock_graphe);
	editeur_graphe->listens(&m_main->contexte);
	editeur_graphe->menu_ajout_noeud(m_menu_ajout_noeud);

	dock_graphe->setWidget(editeur_graphe);
	dock_graphe->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(Qt::RightDockWidgetArea, dock_graphe);

	tabifyDockWidget(dock_graphe, dock_arborescence);

	/* Fais en sorte que l'éditeur de graphe soit visible par défaut. */
	dock_graphe->raise();
}

void FenetrePrincipale::mis_a_jour_menu_fichier_recent()
{
	std::vector<danjo::DonneesAction> donnees_actions;

	danjo::DonneesAction donnees{};
	donnees.attache = "ouvrir_fichier_recent";
	donnees.repondant_bouton = m_repondant_commande;

	for (auto const &fichier_recent : m_main->fichiers_recents()) {
		auto name = QFileInfo(fichier_recent.c_str()).fileName();

		donnees.nom = name.toStdString();
		donnees.metadonnee = fichier_recent;

		donnees_actions.push_back(donnees);
	}

	m_gestionnaire->recree_menu("Projets Récents", donnees_actions);
}
