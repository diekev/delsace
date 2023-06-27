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

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QCoreApplication>
#include <QCursor>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/jorjala.hh"

#include "barre_progres.hh"
#include "chef_execution.hh"
//#include "editrice_arborescence.hh"
#include "editrice_ligne_temps.h"
#include "editrice_noeud.h"
#include "editrice_proprietes.h"
//#include "editrice_rendu.h"
#include "editrice_attributs.hh"
#include "editrice_vue2d.h"
#include "editrice_vue3d.h"
#include "gestion_entreface.hh"
#include "tache.h"

/* ------------------------------------------------------------------------- */

/* Sous-classe de QEvent pour ajouter les évnènements de Jorjala à la boucle
 * d'évènements de Qt. */
class EvenementJorjala : public QEvent {
    JJL::TypeEvenement m_type;

  public:
    static QEvent::Type id_type_qt;

    EvenementJorjala(JJL::TypeEvenement type_evenenemt_jorjala)
        : QEvent(id_type_qt), m_type(type_evenenemt_jorjala)
    {
    }

    JJL::TypeEvenement pour_quoi() const
    {
        return m_type;
    }
};

QEvent::Type EvenementJorjala::id_type_qt;

/* ------------------------------------------------------------------------- */

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
    boite_message.critical(
        données_programme->fenetre_principale, "Erreur", message.vers_std_string().c_str());
    boite_message.setFixedSize(500, 200);
}

static Qt::CursorShape convertis_type_curseur(JJL::TypeCurseur curseur)
{
    switch (curseur) {
        case JJL::TypeCurseur::NORMAL:
            return Qt::CursorShape::ArrowCursor;
        case JJL::TypeCurseur::ATTENTE_BLOQUÉ:
            return Qt::CursorShape::WaitCursor;
        case JJL::TypeCurseur::TÂCHE_ARRIÈRE_PLAN_EN_COURS:
            return Qt::CursorShape::BusyCursor;
        case JJL::TypeCurseur::MAIN_OUVERTE:
            return Qt::CursorShape::OpenHandCursor;
        case JJL::TypeCurseur::MAIN_FERMÉE:
            return Qt::CursorShape::ClosedHandCursor;
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

static void titre_application(void *donnees, JJL::Chaine titre)
{
    auto données_programme = static_cast<DonnéesProgramme *>(donnees);
    données_programme->fenetre_principale->setWindowTitle(titre.vers_std_string().c_str());
}

static void texte_état_application(void *données, JJL::Chaine texte)
{
    auto données_programme = static_cast<DonnéesProgramme *>(données);
    données_programme->fenetre_principale->définit_texte_état(texte.vers_std_string().c_str());
}

static void tache_demaree(void *donnees)
{
    auto données_programme = static_cast<DonnéesProgramme *>(donnees);
    données_programme->task_notifier->signale_debut_evaluation("", 0, 0);
}

static void tache_terminee(void *donnees)
{
    auto données_programme = static_cast<DonnéesProgramme *>(donnees);
    données_programme->task_notifier->signale_fin_tache();
}

/* ------------------------------------------------------------------------- */

void rapporte_démarre_évaluation(void *données, JJL::Chaine message)
{
    static_cast<ChefExecution *>(données)->demarre_evaluation(message.vers_std_string().c_str());
}

void rapporte_progression_chef(void *données, float progression)
{
    static_cast<ChefExecution *>(données)->indique_progression(progression);
}

void rapporte_progression_parallele_chef(void *données, float delta)
{
    static_cast<ChefExecution *>(données)->indique_progression_parallele(delta);
}

bool doit_interrompre_chef(void *données)
{
    return static_cast<ChefExecution *>(données)->interrompu();
}

}  // namespace detail

static void initialise_evenements(JJL::Jorjala &jorjala, FenetrePrincipale *fenetre_principale)
{
    EvenementJorjala::id_type_qt = static_cast<QEvent::Type>(QEvent::registerEventType());

    auto gestionnaire_jjl = jorjala.gestionnaire_fenêtre();
    gestionnaire_jjl.définit_rappel_notification(
        reinterpret_cast<void *>(detail::notifie_observatrices));
    gestionnaire_jjl.définit_rappel_notifie_erreur(
        reinterpret_cast<void *>(detail::notifie_erreur));
    gestionnaire_jjl.définit_rappel_change_curseur(
        reinterpret_cast<void *>(detail::change_curseur));
    gestionnaire_jjl.définit_rappel_restaure_curseur(
        reinterpret_cast<void *>(detail::restaure_curseur));
    gestionnaire_jjl.définit_rappel_titre_application(
        reinterpret_cast<void *>(detail::titre_application));
    gestionnaire_jjl.définit_rappel_tâche_démarrée(
        reinterpret_cast<void *>(detail::tache_demaree));
    gestionnaire_jjl.définit_rappel_tâche_terminée(
        reinterpret_cast<void *>(detail::tache_terminee));
    gestionnaire_jjl.définit_rappel_texte_état_logiciel(
        reinterpret_cast<void *>(detail::texte_état_application));

    auto données_programme = static_cast<DonnéesProgramme *>(gestionnaire_jjl.données());
    données_programme->fenetre_principale = fenetre_principale;
    données_programme->gestionnaire_danjo->parent_dialogue(fenetre_principale);
    données_programme->task_notifier = memoire::loge<TaskNotifier>("TaskNotifier",
                                                                   fenetre_principale);
}

static void initialise_chef_execution(JJL::Jorjala &jorjala, FenetrePrincipale *fenetre_principale)
{
    auto gestionnaire_jjl = jorjala.gestionnaire_fenêtre();
    auto données_programme = static_cast<DonnéesProgramme *>(gestionnaire_jjl.données());

    /* À FAIRE : libère la mémoire. */
    auto chef = memoire::loge<ChefExecution>(
        "ChefExecution", jorjala, données_programme->task_notifier);

    auto chef_jjl = jorjala.chef_exécution();
    chef_jjl.données(chef);

    chef_jjl.définit_rappel_doit_interrompre(
        reinterpret_cast<void *>(detail::doit_interrompre_chef));
    chef_jjl.définit_rappel_rapporte_progression(
        reinterpret_cast<void *>(detail::rapporte_progression_chef));
    chef_jjl.définit_rappel_rapporte_progression_parallèle(
        reinterpret_cast<void *>(detail::rapporte_progression_parallele_chef));
    chef_jjl.définit_rappel_démarre_évaluation(
        reinterpret_cast<void *>(detail::rapporte_démarre_évaluation));
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
    : QMainWindow(parent), m_jorjala(jorjala),
      m_barre_progres(new BarreDeProgres(m_jorjala, this)), m_texte_état(new QLabel("", this))
{
    initialise_evenements(m_jorjala, this);
    initialise_chef_execution(m_jorjala, this);
    initialise_fournisseuse_icône(m_jorjala);

    setWindowTitle("Projet Sans Titre - Jorjala");

    genere_barre_menu();
    genere_menu_prereglages();

    statusBar()->addWidget(m_texte_état);
    définit_texte_état("");

    statusBar()->addWidget(m_barre_progres);
    m_barre_progres->setVisible(false);

    charge_reglages();

    construit_interface_depuis_jorjala();

    /* Nous utilisons un eventFilter pour filtrer les évènements de Jorjala, car
     * surcharger QMainWindow::event() nous fait perdre la capacité de
     * redimensionner les dock widgets. */
    qApp->installEventFilter(this);
}

void FenetrePrincipale::charge_reglages()
{
    QSettings settings;

    auto const &recent_files = settings.value("projet_récents").toStringList();

    for (auto const &file : recent_files) {
        if (QFile(file).exists()) {
            m_jorjala.ajoute_fichier_récent(file.toStdString().c_str());
        }
    }
}

void FenetrePrincipale::ecrit_reglages() const
{
    QSettings settings;
    QStringList recent;

    for (auto fichier_recent : m_jorjala.fichiers_récents()) {
        recent.push_front(fichier_recent.vers_std_string().c_str());
    }

    settings.setValue("projet_récents", recent);
}

void FenetrePrincipale::mis_a_jour_menu_fichier_recent()
{
    dls::tableau<danjo::DonneesAction> donnees_actions;

    danjo::DonneesAction donnees{};
    donnees.attache = "ouvrir_fichier";
    donnees.repondant_bouton = repondant_commande(m_jorjala);

    for (auto fichier_recent : m_jorjala.fichiers_récents()) {
        auto string_fichier_recent = fichier_recent.vers_std_string();
        auto name = QFileInfo(string_fichier_recent.c_str()).fileName();

        donnees.nom = name.toStdString();
        donnees.metadonnee = string_fichier_recent.c_str();

        donnees_actions.ajoute(donnees);
    }

    gestionnaire_danjo(m_jorjala)->recree_menu("Projets Récents", donnees_actions);
}

void FenetrePrincipale::closeEvent(QCloseEvent *)
{
    ecrit_reglages();
}

static QLayout *qlayout_depuis_disposition(JJL::Disposition disposition)
{
    switch (disposition.direction()) {
        case JJL::DirectionDisposition::HORIZONTAL:
        {
            return new QHBoxLayout();
        }
        case JJL::DirectionDisposition::VERTICAL:
        {
            return new QVBoxLayout();
        }
    }
    return nullptr;
}

static Qt::Orientation donne_orientation_qsplitter_disposition(JJL::Disposition disposition)
{
    switch (disposition.direction()) {
        case JJL::DirectionDisposition::HORIZONTAL:
        {
            return Qt::Horizontal;
        }
        case JJL::DirectionDisposition::VERTICAL:
        {
            return Qt::Vertical;
        }
    }
    return Qt::Vertical;
}

static BaseEditrice *qéditrice_depuis_éditrice(JJL::Jorjala &jorjala, JJL::Editrice éditrice)
{
    switch (éditrice.type()) {
        case JJL::TypeEditrice::GRAPHE:
            return new EditriceGraphe(jorjala);
        case JJL::TypeEditrice::PROPRIÉTÉS_NOEUDS:
            return new EditriceProprietes(jorjala);
        case JJL::TypeEditrice::LIGNE_TEMPS:
            return new EditriceLigneTemps(jorjala);
        case JJL::TypeEditrice::RENDU:
            // return new EditriceRendu(jorjala);
            return nullptr;
        case JJL::TypeEditrice::VUE_2D:
            return new EditriceVue2D(jorjala);
        case JJL::TypeEditrice::VUE_3D:
            return new EditriceVue3D(jorjala);
        case JJL::TypeEditrice::ARBORESCENCE:
            // return new EditriceArborescence(m_jorjala);
            return nullptr;
        case JJL::TypeEditrice::ATTRIBUTS:
            return new EditriceAttributs(jorjala);
    }

    return nullptr;
}

static QWidget *génère_interface_disposition(JJL::Jorjala &jorjala,
                                             JJL::Disposition région,
                                             QVector<BaseEditrice *> &éditrices);

static void génère_interface_région(JJL::Jorjala &jorjala,
                                    JJL::RegionInterface &région,
                                    QSplitter *layout,
                                    QVector<BaseEditrice *> &éditrices)
{
    auto qwidget_région = new QWidget();
    layout->addWidget(qwidget_région);

    auto qwidget_région_layout = new QVBoxLayout();
    qwidget_région_layout->setMargin(0);
    qwidget_région->setLayout(qwidget_région_layout);

    if (région.type() == JJL::TypeRegion::CONTENEUR_ÉDITRICE) {
        auto qtab_widget = new QTabWidget(qwidget_région);
        qwidget_région_layout->addWidget(qtab_widget);

        for (auto éditrice : région.éditrices()) {
            auto qéditrice = qéditrice_depuis_éditrice(jorjala, éditrice);
            if (!qéditrice) {
                continue;
            }

            éditrices.push_back(qéditrice);
            qéditrice->ajourne_état(JJL::TypeEvenement::RAFRAICHISSEMENT);

            qtab_widget->addTab(qéditrice, éditrice.nom().vers_std_string().c_str());
        }
    }
    else {
        auto widget = génère_interface_disposition(jorjala, région.disposition(), éditrices);
        qwidget_région_layout->addWidget(widget);
    }
}

static QWidget *génère_interface_disposition(JJL::Jorjala &jorjala,
                                             JJL::Disposition disposition,
                                             QVector<BaseEditrice *> &éditrices)
{
    auto qsplitter = new QSplitter();
    qsplitter->setOrientation(donne_orientation_qsplitter_disposition(disposition));

    for (auto région : disposition.régions()) {
        génère_interface_région(jorjala, région, qsplitter, éditrices);
    }

    auto qlayout = qlayout_depuis_disposition(disposition);
    qlayout->setMargin(0);
    qlayout->addWidget(qsplitter);

    auto qwidget = new QWidget();
    qwidget->setLayout(qlayout);

    return qwidget;
}

void FenetrePrincipale::construit_interface_depuis_jorjala()
{
    auto interface = m_jorjala.donne_interface();
    auto disposition = interface.disposition();

    auto qwidget = génère_interface_disposition(m_jorjala, disposition, m_editrices);
    setCentralWidget(qwidget);
}

bool FenetrePrincipale::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == EvenementJorjala::id_type_qt) {
        auto event_jjl = static_cast<EvenementJorjala *>(event);

        for (auto editrice : m_editrices) {
            editrice->ajourne_état(event_jjl->pour_quoi());
        }

        return true;
    }

    return QWidget::eventFilter(object, event);
}

void FenetrePrincipale::genere_barre_menu()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);

    for (auto const &chemin : chemins_scripts) {
        auto menu = gestionnaire->compile_menu_fichier(donnees, chemin);
        menuBar()->addMenu(menu);
    }

    auto menu_fichiers_recents = gestionnaire_danjo(m_jorjala)->pointeur_menu("Projets Récents");
    connect(menu_fichiers_recents,
            SIGNAL(aboutToShow()),
            this,
            SLOT(mis_a_jour_menu_fichier_recent()));
}

void FenetrePrincipale::genere_menu_prereglages()
{
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);
    m_barre_outil = gestionnaire->compile_barre_outils_fichier(donnees,
                                                               "entreface/menu_prereglage.jo");
    addToolBar(Qt::TopToolBarArea, m_barre_outil);
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
    m_barre_progres->ajourne_valeur(static_cast<int>(progres * 100.0f));
}

void FenetrePrincipale::tache_terminee()
{
    m_barre_progres->setVisible(false);
}

void FenetrePrincipale::evaluation_debutee(const QString &message, int execution, int total)
{
    m_barre_progres->ajourne_valeur(0);
    m_barre_progres->setVisible(true);
    m_barre_progres->ajourne_message(message, execution, total);
}

void FenetrePrincipale::définit_texte_état(const QString &texte)
{
    m_texte_état->setVisible(texte != "");
    m_texte_état->setText(texte);
}
