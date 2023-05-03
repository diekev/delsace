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

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QCursor>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#include <QMessageBox>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/fichier.hh"

#include "coeur/jorjala.hh"
//#include "coeur/tache.h"

#include "barre_progres.hh"
//#include "editrice_arborescence.hh"
#include "editrice_ligne_temps.h"
#include "editrice_noeud.h"
#include "editrice_proprietes.h"
//#include "editrice_rendu.h"
#include "editrice_vue2d.h"
#include "editrice_vue3d.h"

/* ------------------------------------------------------------------------- */

/* Sous-classe de QEvent pour ajouter les évnènements de Jorjala à la boucle
 * d'évènements de Qt. */
class EvenementJorjala : public QEvent {
    JJL::TypeEvenement m_type;

public:
    static QEvent::Type id_type_qt;

    EvenementJorjala(JJL::TypeEvenement type_evenenemt_jorjala)
        : QEvent(id_type_qt)
        , m_type(type_evenenemt_jorjala)
    {
    }

    JJL::TypeEvenement pour_quoi() const
    {
        return m_type;
    }
};

QEvent::Type EvenementJorjala::id_type_qt;

namespace detail {

static void notifie_observatrices(void *donnees, JJL::TypeEvenement evenement)
{
    auto données_programme = static_cast<DonnéesProgramme *>(donnees);
    auto event = new EvenementJorjala(evenement);
    QCoreApplication::postEvent(données_programme->fenetre_principale, event);
}

static void notifie_erreur(void *donnees, JJL::Chaine message)
{
    auto données_programme = static_cast<DonnéesProgramme *>(donnees);
    QMessageBox boite_message;
    boite_message.critical(données_programme->fenetre_principale, "Erreur", message.vers_std_string().c_str());
    boite_message.setFixedSize(500, 200);
}

static Qt::CursorShape convertis_type_curseur(JJL::TypeCurseur curseur)
{
    switch (curseur) {
        case JJL::TypeCurseur::NORMAL: return Qt::CursorShape::ArrowCursor;
        case JJL::TypeCurseur::ATTENTE_BLOQUÉ: return Qt::CursorShape::WaitCursor;
        case JJL::TypeCurseur::TÂCHE_ARRIÈRE_PLAN_EN_COURS: return Qt::CursorShape::BusyCursor;
        case JJL::TypeCurseur::MAIN_OUVERTE: return Qt::CursorShape::OpenHandCursor;
        case JJL::TypeCurseur::MAIN_FERMÉE: return Qt::CursorShape::ClosedHandCursor;
    }

    return Qt::CursorShape::ArrowCursor;
}

static void change_curseur(void *donnees, JJL::TypeCurseur curseur)
{
  QGuiApplication::setOverrideCursor(QCursor(convertis_type_curseur(curseur)));
}

static void restaure_curseur(void *donnees, JJL::TypeCurseur curseur)
{
  QGuiApplication::restoreOverrideCursor();
}

}

static void initialise_evenements(JJL::Jorjala &jorjala, FenetrePrincipale *fenetre_principale)
{
    EvenementJorjala::id_type_qt = static_cast<QEvent::Type>(QEvent::registerEventType());

    auto gestionnaire_jjl = jorjala.gestionnaire_fenêtre();
    gestionnaire_jjl.mute_rappel_notification(reinterpret_cast<void *>(detail::notifie_observatrices));
    gestionnaire_jjl.mute_rappel_notifie_erreur(reinterpret_cast<void *>(detail::notifie_erreur));
    gestionnaire_jjl.définit_rappel_change_curseur(reinterpret_cast<void *>(detail::change_curseur));
    gestionnaire_jjl.définit_rappel_restaure_curseur(reinterpret_cast<void *>(detail::restaure_curseur));

    auto données_programme = static_cast<DonnéesProgramme *>(gestionnaire_jjl.données());
    données_programme->fenetre_principale = fenetre_principale;
}

/* ------------------------------------------------------------------------- */

static const char *chemins_scripts[] = {
	"entreface/menu_fichier.jo",
	"entreface/menu_edition.jo",
	"entreface/menu_ajouter_noeud_composite.jo",
	"entreface/menu_ajouter_noeud_detail.jo",
	"entreface/menu_ajouter_noeud_objet.jo",
	"entreface/menu_ajouter_noeud_simulation.jo",
	"entreface/menu_debogage.jo",
};

enum {
	EDITRICE_ARBORESCENCE,
	EDITRICE_PROPRIETE,
	EDITRICE_GRAPHE,
	EDITRICE_LIGNE_TEMPS,
	EDITRICE_RENDU,
	EDITRICE_VUE2D,
	EDITRICE_VUE3D,
};

FenetrePrincipale::FenetrePrincipale(JJL::Jorjala &jorjala, QWidget *parent)
	: QMainWindow(parent)
	, m_jorjala(jorjala)
    , m_barre_progres(new BarreDeProgres(m_jorjala, this))
{
//	jorjala.fenetre_principale = this;
//	jorjala.notifiant_thread = memoire::loge<TaskNotifier>("TaskNotifier", this);
//	jorjala.gestionnaire_entreface->parent_dialogue(this);

    initialise_evenements(m_jorjala, this);

	genere_barre_menu();
	genere_menu_prereglages();

    statusBar()->addWidget(m_barre_progres);
    m_barre_progres->setVisible(false);

	auto dock_vue2D = ajoute_dock("Vue 2D", EDITRICE_VUE2D, Qt::LeftDockWidgetArea);
	ajoute_dock("Vue 3D", EDITRICE_VUE3D, Qt::LeftDockWidgetArea, dock_vue2D);
	dock_vue2D->raise();

	ajoute_dock("Grapĥe", EDITRICE_GRAPHE, Qt::LeftDockWidgetArea);

	auto dock_arbre = ajoute_dock("Arborescence", EDITRICE_ARBORESCENCE, Qt::RightDockWidgetArea);
	ajoute_dock("Propriétés", EDITRICE_PROPRIETE, Qt::RightDockWidgetArea, dock_arbre);
	dock_arbre->raise();

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
            // m_jorjala.ajoute_fichier_recent(file.toStdString());
		}
	}
}

void FenetrePrincipale::ecrit_reglages() const
{
	QSettings settings;
	QStringList recent;

//	for (auto const &fichier_recent : m_jorjala.fichiers_recents()) {
//		recent.push_front(fichier_recent.c_str());
//	}

	settings.setValue("projet_récents", recent);
}

void FenetrePrincipale::mis_a_jour_menu_fichier_recent()
{
//	dls::tableau<danjo::DonneesAction> donnees_actions;

//	danjo::DonneesAction donnees{};
//	donnees.attache = "ouvrir_fichier_recent";
//	donnees.repondant_bouton = repondant_commande(m_jorjala);

//	for (auto const &fichier_recent : m_jorjala.fichiers_recents()) {
//		auto name = QFileInfo(fichier_recent.c_str()).fileName();

//		donnees.nom = name.toStdString();
//		donnees.metadonnee = fichier_recent;

//		donnees_actions.ajoute(donnees);
//	}

//	m_jorjala.gestionnaire_entreface->recree_menu("Projets Récents", donnees_actions);
}

void FenetrePrincipale::closeEvent(QCloseEvent *)
{
    ecrit_reglages();
}

bool FenetrePrincipale::event(QEvent *event)
{
    if (event->type() == EvenementJorjala::id_type_qt) {
        auto event_jjl = static_cast<EvenementJorjala *>(event);

        for (auto editrice : m_editrices) {
            editrice->ajourne_etat(static_cast<int>(event_jjl->pour_quoi()));
        }

        return true;
    }

    return QWidget::event(event);
}

void FenetrePrincipale::genere_barre_menu()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);

    for (auto const &chemin : chemins_scripts) {
        auto menu = gestionnaire->compile_menu_fichier(donnees, chemin);
        menuBar()->addMenu(menu);
    }

//	auto menu_fichiers_recents = m_jorjala.gestionnaire_entreface->pointeur_menu("Projets Récents");
//	connect(menu_fichiers_recents, SIGNAL(aboutToShow()),
//			this, SLOT(mis_a_jour_menu_fichier_recent()));
}

void FenetrePrincipale::genere_menu_prereglages()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);
    m_barre_outil = gestionnaire->compile_barre_outils_fichier(donnees, "entreface/menu_prereglage.jo");
    addToolBar(Qt::TopToolBarArea, m_barre_outil);
}

QDockWidget *FenetrePrincipale::ajoute_dock(QString const &nom, int type, int aire, QDockWidget *premier)
{
    BaseEditrice *editrice = nullptr;

    switch (type) {
        case EDITRICE_GRAPHE:
            editrice = new EditriceGraphe(m_jorjala);
            break;
        case EDITRICE_PROPRIETE:
            editrice = new EditriceProprietes(m_jorjala);
            break;
        case EDITRICE_LIGNE_TEMPS:
            editrice = new EditriceLigneTemps(m_jorjala);
            break;
        case EDITRICE_RENDU:
            // editrice = new EditriceRendu(m_jorjala);
            break;
        case EDITRICE_VUE2D:
            editrice = new EditriceVue2D(m_jorjala);
            break;
        case EDITRICE_VUE3D:
            // editrice = new EditriceVue3D(m_jorjala);
            break;
        case EDITRICE_ARBORESCENCE:
            // editrice = new EditriceArborescence(m_jorjala);
            break;
    }

	auto dock = new QDockWidget(nom, this);
	dock->setAttribute(Qt::WA_DeleteOnClose);

    if (editrice) {
        m_editrices.push_back(editrice);
        editrice->ajourne_etat(static_cast<int>(JJL::TypeEvenement::RAFRAICHISSEMENT));
        dock->setWidget(editrice);
    }
	dock->setAllowedAreas(Qt::AllDockWidgetAreas);

	addDockWidget(static_cast<Qt::DockWidgetArea>(aire), dock);

	if (premier) {
		tabifyDockWidget(premier, dock);
	}

	return dock;
}

void FenetrePrincipale::image_traitee()
{
    m_jorjala.notifie_observatrices(JJL::TypeEvenement::IMAGE | JJL::TypeEvenement::TRAITÉ);
}

void FenetrePrincipale::signale_proces(int quoi)
{
    m_jorjala.notifie_observatrices(static_cast<JJL::TypeEvenement>(quoi));
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

void FenetrePrincipale::evaluation_debutee(const char *message, int execution, int total)
{
    m_barre_progres->ajourne_valeur(0);
    m_barre_progres->ajourne_message(message, execution, total);
}
